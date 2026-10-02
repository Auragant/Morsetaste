// SPDX-License-Identifier: GPL-3.0-only
// Test the exact privileged script embedded in the workflow, without network calls.
// Only run a trusted checkout: node:vm is a test context, NOT a security sandbox.
// The fixed repository workflow is intentionally executable source, like this test.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const workflow = fs.readFileSync(path.join(__dirname, '../.github/workflows/security.yml'), 'utf8');
const match = workflow.match(/^ {10}script: \|\r?\n([\s\S]+)$/m);
assert.ok(match, 'notification script exists');
const script = match[1].replace(/^ {12}/gm, '');
const marker = '<!-- morsebridge-security-monitor:v1 -->';
const openIssue = {number: 7, body: marker, user: {type: 'Bot'}};
const good = {cppcheck: {result: 'success'}, secrets: {result: 'success'}, tests: {result: 'success'}};
const bad = {...good, secrets: {result: 'failure'}};

async function run(results, issues = [], options = {}) {
  const calls = [];
  const issuesApi = {};
  for (const name of ['create', 'createComment', 'update']) {
    issuesApi[name] = async args => {
      calls.push({name, args});
      return {data: {html_url: 'https://example.invalid/test-issue'}};
    };
  }
  issuesApi.listForRepo = () => {};
  const sandbox = {
    context: {repo: {owner: 'owner', repo: 'repo'}, serverUrl: 'https://github.com', runId: 42,
      sha: 'current', payload: {repository: {default_branch: 'main'}}},
    process: {env: {CHECK_RESULTS: JSON.stringify(results), TEST_NOTIFICATION: options.test ? 'true' : 'false'}},
    core: {notice: () => {}},
    github: {
      paginate: async () => issues,
      rest: {issues: issuesApi, repos: {getBranch: async () => ({data: {commit: {sha: options.stale ? 'newer' : 'current'}}})}}
    }
  };
  await vm.runInNewContext(`(async () => {\n${script}\n})()`, sandbox);
  return calls;
}

(async () => {
  let calls = await run(good);
  assert.equal(calls.length, 0, 'clean run without issue is quiet');
  calls = await run(bad);
  assert.equal(calls.length, 1);
  assert.equal(calls[0].name, 'create');
  assert.equal(calls[0].args.assignees[0], 'owner', 'issue is assigned');
  assert.ok(calls[0].args.body.includes('@owner'), 'owner is mentioned');
  assert.ok(calls[0].args.body.includes('actions/runs/42'), 'run linked');
  assert.ok(calls[0].args.body.includes('secrets'), 'failed scanner identified');
  calls = await run(bad, [openIssue]);
  assert.equal(calls.length, 1);
  assert.equal(calls[0].name, 'createComment', 'repeated failure reuses issue');
  calls = await run(good, [openIssue]);
  assert.equal(calls[1].name, 'update');
  assert.equal(calls[1].args.state, 'closed', 'green follow-up resolves issue');
  calls = await run({...good, tests: {result: 'cancelled'}}, [openIssue]);
  assert.equal(calls.length, 0, 'cancelled scan cannot resolve issue');
  calls = await run({...good, cppcheck: {result: 'failure'}, tests: {result: 'skipped'}}, [openIssue]);
  assert.equal(calls[0].name, 'createComment', 'known failure is still reported in partial run');
  calls = await run(good, [openIssue], {stale: true});
  assert.equal(calls.length, 0, 'stale green cannot resolve newer issue');
  calls = await run(bad, [], {stale: true});
  assert.equal(calls.length, 0, 'stale failure cannot create outdated issue');
  calls = await run(bad, [{...openIssue, user: {type: 'User'}}]);
  assert.equal(calls[0].name, 'create', 'do not mutate a user-created issue');
  calls = await run(bad, [{...openIssue, pull_request: {}}]);
  assert.equal(calls[0].name, 'create', 'do not mutate pull requests');
  calls = await run(good, [], {test: true});
  assert.equal(calls[0].args.title, '[TEST] GitHub-Security-Benachrichtigung');
  assert.equal(calls[0].args.assignees[0], 'owner');
  console.log('PASS: 11 notification-routing scenarios (no GitHub writes)');
})().catch(error => { console.error(error); process.exitCode = 1; });

// SPDX-License-Identifier: GPL-3.0-only
// Exercise the exact privileged workflow script with a fake GitHub API.
// Import a static function; never evaluate JavaScript read from the YAML file.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const notify = require('../.github/scripts/compatibility-notify.cjs');
const workflow = fs.readFileSync(path.join(__dirname, '../.github/workflows/compatibility.yml'), 'utf8');
const match = workflow.match(/^ {10}script: \|\r?\n([\s\S]+)$/m);
assert.ok(match, 'notification script exists');
const script = match[1].replace(/^ {12}/gm, '').replace(/\r\n/g, '\n').trim();
const invocation = 'await notify({github, context, core, results: JSON.parse(process.env.CHECK_RESULTS)});';
assert.equal(script, `${notify.toString().replace(/\r\n/g, '\n')}\n${invocation}`,
  'the tested static function is identical to the privileged workflow script');
const marker = '<!-- morsebridge-compatibility-monitor:v1 -->';
const openIssue = {number: 7, body: marker, user: {login: 'github-actions[bot]'}};
const good = {windows: {result: 'success'}, sdk: {result: 'success'}};
const bad = {...good, windows: {result: 'failure'}};

async function run(results, issues = [], stale = false) {
  const calls = [];
  const issuesApi = {listForRepo: () => {}};
  for (const name of ['create', 'createComment', 'update']) {
    issuesApi[name] = async args => { calls.push({name, args}); };
  }
  const sandbox = {
    context: {repo: {owner: 'owner', repo: 'repo'}, serverUrl: 'https://github.com', runId: 42,
      sha: 'current', payload: {repository: {default_branch: 'main'}}},
    core: {notice: () => {}},
    github: {paginate: async () => issues, rest: {issues: issuesApi,
      repos: {getBranch: async () => ({data: {commit: {sha: stale ? 'newer' : 'current'}}})}}}
  };
  await notify({...sandbox, results});
  return calls;
}

(async () => {
  let calls = await run(good);
  assert.equal(calls.length, 0, 'healthy run is quiet');
  calls = await run(bad);
  assert.equal(calls.length, 1);
  assert.equal(calls[0].name, 'create');
  assert.deepEqual(Array.from(calls[0].args.assignees), ['owner']);
  assert.ok(calls[0].args.body.includes('@owner'));
  assert.ok(calls[0].args.body.includes('windows'));
  assert.ok(calls[0].args.body.includes('actions/runs/42'));
  calls = await run({...good, sdk: {result: 'failure'}});
  assert.ok(calls[0].args.body.includes('sdk'), 'SDK failure reported');
  calls = await run(bad, [openIssue]);
  assert.equal(calls.length, 1);
  assert.equal(calls[0].name, 'createComment', 'failure reuses the open issue');
  calls = await run(good, [openIssue]);
  assert.deepEqual(calls.map(c => c.name), ['createComment', 'update']);
  assert.equal(calls[1].args.state, 'closed');
  for (const result of ['cancelled', 'skipped']) {
    calls = await run({...good, windows: {result}}, [openIssue]);
    assert.equal(calls.length, 0, `${result} run cannot resolve a failure`);
  }
  calls = await run({windows: {result: 'success'}}, [openIssue]);
  assert.equal(calls.length, 0, 'missing SDK result cannot resolve a failure');
  calls = await run(bad, [openIssue], true);
  assert.equal(calls.length, 0, 'stale failure cannot update an issue');
  calls = await run(good, [openIssue], true);
  assert.equal(calls.length, 0, 'stale success cannot close an issue');
  calls = await run(bad, [{...openIssue, body: '<!-- morsebridge-security-monitor:v1 -->'}]);
  assert.equal(calls[0].name, 'create', 'security issues remain separate');
  calls = await run(bad, [{...openIssue, user: {login: 'other-user'}}]);
  assert.equal(calls[0].name, 'create', 'a human issue is not modified');
  calls = await run(bad, [{...openIssue, pull_request: {}}]);
  assert.equal(calls[0].name, 'create', 'a pull request is not modified');
  console.log('PASS: 13 compatibility notification scenarios');
})().catch(error => { console.error(error); process.exit(1); });

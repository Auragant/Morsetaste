// SPDX-License-Identifier: GPL-3.0-only
// Static testable source. The workflow embeds this exact function so its
// privileged job needs no checkout; tests verify equality before exercising it.
module.exports = async function notify({github, context, core, results}) {
  const repo = context.repo;
  const marker = '<!-- morsebridge-compatibility-monitor:v1 -->';
  const runUrl = `${context.serverUrl}/${repo.owner}/${repo.repo}/actions/runs/${context.runId}`;
  const failed = Object.entries(results).filter(([, v]) => v.result === 'failure').map(([k]) => k);
  const clean = ['windows', 'sdk'].every(k => results[k]?.result === 'success');
  const {data: branch} = await github.rest.repos.getBranch({...repo, branch: context.payload.repository.default_branch});
  if (branch.commit.sha !== context.sha) {
    core.notice('Veralteter Prüflauf: Issue-Status bleibt unverändert.');
    return;
  }
  if (!failed.length && !clean) return;
  const issues = await github.paginate(github.rest.issues.listForRepo, {...repo, state: 'open', per_page: 100});
  const existing = issues.find(i => !i.pull_request && i.user?.login === 'github-actions[bot]' && i.body?.includes(marker));
  if (failed.length) {
    const body = `${marker}\n@${repo.owner}: Die Windows-Kompatibilitätsprüfung ist fehlgeschlagen.\n\nBetroffene Prüfungen: **${failed.join(', ')}**.\n\n[Logs, Windows-Versionen und Prüfergebnisse](${runUrl}). Bitte prüfen, ob Windows-/SDK-Änderungen, Programmfehler oder ein Tool-/Downloadproblem die Ursache sind. Dieser Fehler beweist noch keine Inkompatibilität.\n\nEin vollständiger grüner Folgeprüflauf schließt dieses Issue automatisch.`;
    if (existing) {
      await github.rest.issues.createComment({...repo, issue_number: existing.number, body});
    } else {
      await github.rest.issues.create({...repo, title: '[Kompatibilität] Windows-Prüfung benötigt Aufmerksamkeit', body, assignees: [repo.owner]});
    }
  } else if (existing) {
    await github.rest.issues.createComment({...repo, issue_number: existing.number, body: `Beide Windows-Laufzeitprüfungen und der Microsoft-SDK-Build sind wieder erfolgreich: [Prüflauf](${runUrl}). Hardware und Treiber des eigenen PCs sind damit nicht geprüft.`});
    await github.rest.issues.update({...repo, issue_number: existing.number, state: 'closed', state_reason: 'completed'});
  }
};

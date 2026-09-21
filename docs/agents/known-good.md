# Cut a known-good tag

After the main merge and self-review, set RECIPE_REPOSITORY to this local checkout,
VERIFIED_COMMIT to the full current main hash, and NEW_KNOWN_GOOD to a new
`known-good-YYYY-MM-DD` name. `gh` must be authenticated to msetaro/aftershock.
The command rechecks the current main hash and its latest push workflows, then
creates an annotated local tag. It never pushes, replaces or deletes a tag.

```sh
python3 - <<'PY'
import json, os, re, subprocess
from pathlib import Path
repo = Path(os.environ['RECIPE_REPOSITORY']).resolve()
commit, tag = os.environ['VERIFIED_COMMIT'], os.environ['NEW_KNOWN_GOOD']
assert re.fullmatch(r'[0-9a-f]{40}', commit), 'use a full commit hash'
assert re.fullmatch(r'known-good-\d{4}-\d{2}-\d{2}', tag), 'use a new dated known-good tag'
def api(path):
    return json.loads(subprocess.check_output(['gh', 'api', 'repos/msetaro/aftershock/'+path], text=True))
def git(*args):
    return subprocess.check_output(['git', *args], cwd=repo, text=True).strip()
assert not git('status', '--porcelain'), 'commit or set aside local edits first'
assert git('rev-parse', commit+'^{commit}') == commit
assert api('commits/main')['sha'] == commit, 'main advanced; recheck the new tree'
for workflow, minimum in (('build.yml', 17), ('regression.yml', 10)):
    runs = api('actions/workflows/'+workflow+'/runs?branch=main&event=push&head_sha='+commit+'&per_page=1')['workflow_runs']
    assert runs and runs[0]['head_sha'] == commit, 'missing main workflow'
    run = runs[0]
    assert run['status'] == 'completed' and run['conclusion'] == 'success', 'workflow is not green'
    data = api('actions/runs/'+str(run['id'])+'/jobs?per_page=100')
    assert data['total_count'] == len(data['jobs']), 'job pagination changed; inspect the complete gate'
    # Only the explicitly deferred job and event-only release updater are exempt.
    jobs = [job for job in data['jobs'] if not job['name'].startswith('fuzz') and job['name'] != 'update-release']
    assert len(jobs) >= minimum and all(job['status'] == 'completed' and job['conclusion'] == 'success' for job in jobs), 'required checks must all succeed'
assert api('commits/main')['sha'] == commit, 'main advanced during verification'
assert not git('tag', '--list', tag), 'known-good tags are immutable; choose a new date'
subprocess.run(['git', '-c', 'tag.gpgsign=false', 'tag', '-a', tag, commit, '-m', 'Verified main '+commit], cwd=repo, check=True)
print(json.dumps({'tag':tag, 'commit':commit, 'published':False}))
PY
```

Publishing a new rollback point is a separate repository-only action after checking
the resulting tag. CI executes this recipe against a private Git history with API
fixtures, then checks that red/skipped required jobs and an existing tag are rejected.
No production tag is created by recipe tests.

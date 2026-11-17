# PSP3DS unattended milestone rotation

The scheduler is installed at `.github/workflows/psp3ds-unattended.yml`.
It is intentionally **inactive** until a coding service has been authorized.

## Operation

- Every six hours (and on manual dispatch), three workstreams can start concurrently:
  `memory`, `interpreter`, and `platform`.
- Each workstream selects one small improvement from the checked-in TODO and
  adds a focused regression test. It cannot modify workflows or vendored assets.
- If its `psp3ds-bot/<workstream>` pull request is still open, the workstream
  skips the cycle rather than stacking more changes on an unreviewed branch.
- The coding job only has read access to the repository. It serializes a source
  patch. A distinct publishing job applies and checks that patch before opening
  a pull request.
- A failed coding job should not prevent other successful workstreams from
  publishing their candidate patches. No patch is merged automatically.
- On the next eligible cycle after its pull request is merged/closed, the
  workstream may select another pending task. It does not guarantee that
  an entire PSP emulator can be completed autonomously.
- Actual milestone completion requires passing tests, successful integration,
  and documented device-specific evidence; no commercial PSP game boots yet.

## Activation prerequisites (one-time account authorization)

1. Authorize an API provider by storing a billable OpenAI API key in the
   repository Actions secret named `OPENAI_API_KEY`. ChatGPT subscription
   alone does not supply a GitHub Actions API key. Never commit credentials.
2. Enable repository variable `PSP3DS_UNATTENDED_ENABLED` with value `true`.
3. Ensure GitHub Actions are permitted to create pull requests and have
   appropriate repository permissions. If not, the publish stage fails safely.

To suspend further coding and API spending, set the variable to `false`.
Existing pull requests remain available for inspection and test verification.
The workflow has no mechanism to mint API credentials, alter account billing,
or bypass GitHub's authorization settings.

## Safety and limitations

- Three concurrent jobs may consume paid inference/Actions resources.
- No unattended changes go straight to `main`; proposed changes must be
  reviewed, and test failures must be fixed before merging.
- This workflow is *not* a substitute for physical 3DS/DSi validation.
- An empty or exhausted TODO is not itself proof of compatibility or
  project completion. The schedule must be disabled when development ends.

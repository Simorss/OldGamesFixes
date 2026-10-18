# Old Games Fixes website

The public project catalogue lives at https://simorss.github.io/OldGamesFixes/.
It uses plain HTML, CSS and JavaScript, with no build dependencies, tracking or
external fonts. All interface text is in English.

## Editing

- `index.html`: project cards, features, release versions and download links.
- `assets/style.css`: responsive layout and visual design.
- `assets/app.js`: category filters, search and the screenshot comparison.
- `favicon.svg`: site icon.

When adding a project, update the project totals, filter counts and searchable
keywords as well as the card. Download links are deliberately pinned to the
project's own release tag and asset; this keeps bonus tools separate from fixes.

## Publishing

The existing `.github/workflows/emergency4-comparison-pages.yml` workflow
publishes this directory at the site root, the original Emergency 4 comparison
at `/emergency4/`, and the original screenshots at `/screenshots/`.
Changes under `site/` automatically trigger deployment through GitHub Actions.
The game release packages are built separately and do not include website files.

To preview locally, serve the deployment directory with any static HTTP server.
The workflow assembles that directory from `site/`,
`Emergency4/comparison/index.html` and `Emergency4/screenshots/*.png`.

100% of the work on this website was done through **GPT-6.1 Sol**.
The page structure takes inspiration from the project catalogue at
https://github.fusionfix.io/wfp; its code and assets are not bundled here.

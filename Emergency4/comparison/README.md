# Interactive screenshot comparison

This static page compares the original 3440×1440 screenshots using a draggable
divider. It supports mouse/touch input, a keyboard-accessible range control,
full-image presets and fullscreen viewing.

GitHub README and release descriptions display the static screenshots. Their
interactive comparison link opens the GitHub Pages version:

https://simorss.github.io/OldGamesFixes/emergency4/

## Publishing

In repository **Settings → Pages → Build and deployment → Source**, choose
**GitHub Actions**. Then run the **Emergency 4 screenshot comparison** workflow
from the Actions tab. Subsequent changes to this page or its screenshots
publish automatically.

The connector used to manage repository contents does not have the
administrative permission needed to enable Pages settings. Page activation
must be completed by the repository owner.

Screenshots and the page are not included in the game ZIP. The original PNG
files are kept in `../screenshots/` without resizing, cropping or retouching.

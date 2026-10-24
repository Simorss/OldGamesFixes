# Interactive screenshot comparison

This static page compares the original 3440×1440 screenshots using a draggable
divider. It supports mouse/touch input, a keyboard-accessible range control,
full-image presets and fullscreen viewing.

GitHub README and release descriptions display the static screenshots. Their
interactive comparison link opens the GitHub Pages version:

https://simorss.github.io/OldGamesFixes/emergency4/

The [project catalogue](https://simorss.github.io/OldGamesFixes/) also includes a
compact comparison directly inside the Emergency 4 project card.

## Publishing

GitHub Pages is configured to use GitHub Actions. The **Old Games Fixes website**
workflow publishes the homepage, this comparison and the screenshots together.
Changes to `site/`, this page or its screenshots deploy automatically. The
workflow can also be run manually from the Actions tab.

Screenshots and the website are not included in the game ZIP. The original image
files are kept in `../screenshots/` without resizing, cropping or retouching.

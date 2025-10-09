<div align="center">
	<img alt="InfiniPaint Logo" src="https://raw.githubusercontent.com/ErrorAtLine0/infinipaint/refs/heads/main/logo.svg" width=150/>
	<h3>Infinite Space, Infinite Zoom, Collaborative Drawing</h3>
	<p>
		<a href="https://infinipaint.com">🔗 Website</a> -
		<a href="https://infinipaint.com/try.html">🌐 Try in Browser</a> -
		<a href="https://github.com/ErrorAtLine0/infinipaint/blob/main/docs/MANUAL.md">📕 Usage Manual</a> -
		<a href="https://github.com/ErrorAtLine0/infinipaint/blob/main/docs/BUILDING.md">⚒️ Build Manual</a>
	</p>
	<p>
		<a href="https://opensource.org/license/mit"><img alt="MIT License" src="https://img.shields.io/badge/license-MIT-blue"/></a>
		<a href="https://github.com/ErrorAtLine0/infinipaint/releases/latest"><img alt="Release" src="https://img.shields.io/github/release/erroratline0/infinipaint.svg"/></a>
		<a href="https://github.com/ErrorAtLine0/infinipaint"><img alt="Github Stars" src="https://img.shields.io/github/stars/erroratline0/infinipaint"/></a>
	</p>
</div>

## InfiniPaint

InfiniPaint is a **collaborative, infinite** canvas note-taking/drawing app. Unlike many other infinite canvas apps, **there is no zoom-in or zoom-out limit** (at least up until the point your computer runs out of memory). This means that this app is very good at things such as drawing sketches of the solar system to scale, or just drawing any massive objects with tiny details. Of course, even though this is a feature, this app is also perfectly well suited for use as a normal canvas.

You can try the web version of this app at [infinipaint.com](https://infinipaint.com) (requires a WebGL2 capable browser). The web version was compiled from C++ to Javascript using Emscripten, and may contain some bugs, so if possible, please consider downloading the native version for a better experience.

This app was inspired by [Lorien](https://github.com/mbrlabs/Lorien), another great infinite canvas program.

## Installation
- **Windows:** Available in [Releases](https://github.com/erroratline0/infinipaint/releases/latest) page. There are two versions:
  	- Installer version. Recommended, since it automatically associates .infpnt files with InfiniPaint
  	- Portable version, which is a zip file containing the executable and data files, and stores configuration files next to the executable. Not recommended unless you are specifically looking for a portable version
- **macOS:** Available in [Releases](https://github.com/erroratline0/infinipaint/releases/latest) page. Only for Apple Silicon
- **Linux:** It is recommended that you download from [Flathub](https://flathub.org/en/apps/com.infinipaint.infinipaint) to stay updated. Otherwise, there are flatpak bundles available for download on the [Releases](https://github.com/erroratline0/infinipaint/releases/latest) page for both x86_64 and arm64

## Screenshots/GIFs

<img alt="Zoom Forever Showcase" src="https://github.com/ErrorAtLine0/infinipaint/blob/main/docs/showcase/zoomforever.png?raw=true" width="100%"/>
<img alt="Sketches Showcase" src="https://github.com/ErrorAtLine0/infinipaint/blob/main/docs/showcase/sketches.png?raw=true" width="100%"/>
<img alt="Online Collaboration Showcase" src="https://github.com/ErrorAtLine0/infinipaint/blob/main/docs/showcase/tictactoe.png?raw=true" width="100%"/>
<img alt="Images Showcase" src="https://github.com/ErrorAtLine0/infinipaint/blob/main/docs/showcase/images.png?raw=true" width="100%"/>
<img alt="Right Click Menu Showcase" src="https://github.com/ErrorAtLine0/infinipaint/blob/main/docs/showcase/rotate.png?raw=true" width="100%"/>
<img alt="Zoom Showcase" src="https://github.com/ErrorAtLine0/infinipaint/blob/main/docs/showcase/zoom.gif?raw=true" width="100%"/>
<img alt="Bookmark Showcase" src="https://github.com/ErrorAtLine0/infinipaint/blob/main/docs/showcase/bookmarkslideshow.gif?raw=true" width="100%"/>

## Features

- Infinite canvas, with infinite zoom
- Open online lobbies for collaboration
	- Text chat with others in the lobby
	- Jump to the location of other players through the player list
	- See other members draw in real time
	- Although this is a feature, this app can also be used offline
- Graphics tablet support (Pressure sensitive brush and eraser detection)
- Saveable color palettes
- Quick menu usable by right clicking on the canvas, which can be used to:
	- Quickly change brush colors using the currently selected color palette
	- Rotate the canvas
- Place bookmarks on the canvas to jump to later
- Undo/Redo
- PNG, JPG, WEBP export of specific parts of the canvas at any resolution (Screenshot)
- SVG export of specific parts of the canvas (Screenshot)
- Transform (Move, Scale, Rotate) any object on the canvas (Rectangle Select Tool/Lasso Select Tool)
- Display Images and animated GIFs on the canvas
	- Note: May take a lot of memory to store and display images compared to other objects, especially GIFs
- Hide (or unhide) the UI by pressing Tab
- Remappable keybinds
- Create custom UI themes
- Place infinite square grids on the canvas as guides for drawing
    - Grids come with various properties, including changing color, and displaying coordinate axes.
- Other tools: Textbox, Rectangle, Circle, Eye dropper/color picker, Edit/cursor
- Can copy/paste selected objects (Ctrl-C Ctrl-V). This can also be done between different files, as long as they're open in different tabs in the same window.
- Any file can be placed on the canvas. Using the edit tool, you can download any file/image on the canvas to your computer. This can be used as a sort of file sharing mechanism in a collaborative canvas (should probably only use this for small files...)

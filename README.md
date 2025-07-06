# ![InfiniPaint Logo](logo.svg) InfiniPaint



InfiniPaint is a **collaborative, infinite** canvas note-taking/drawing app. Unlike many other infinite canvas apps, although there is a specific zoom-in level limit, there is **no zoom-out limit** (at least up until the point your computer runs out of memory). This means that this app is very good at things such as drawing sketches of the solar system to scale, or just drawing any massive objects with tiny details. Of course, even though this is a feature, this app is also perfectly well suited for use as a normal canvas.

You can try the web version of this app at [infinipaint.com](https://infinipaint.com) (requires a WebGL2 capable browser). The web version was compiled from C++ to Javascript using Emscripten, and may contain some bugs, so if possible, please consider downloading the native version for a better experience.

This program is a work in progress, so I cannot guarantee that files created in this version will open in future versions.

This app was inspired by [Lorien](https://github.com/mbrlabs/Lorien), another great infinite canvas program.

##Features

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
- PNG, JPG, WEBP export of specific parts of the canvas at any resolution (Screenshot Tool)
- Transform (Move, Scale, Rotate) any object on the canvas (Rectangle Select Tool)
- Display Images and animated GIFs on the canvas
	- Note: May take a lot of memory to store and display images compared to other objects, especially GIFs
- Hide (or unhide) the UI by pressing Tab
- Remappable keybinds
- Create custom UI themes
- Static square grid at initial zoom level
- Other tools: Textbox, Rectangle, Circle, Eye dropper/color picker, Edit/cursor
- Can copy/paste objects selected with the Rectangle Select tool (Ctrl-C Ctrl-V). This can also be done between different files, as long as they're open in different tabs in the same window.
- Any file can be placed on the canvas, and using the edit tool, you can download any file/image on the canvas to your computer. This can be used as a sort of file sharing mechanism in a collaborative canvas (should probably only use this for small files...)

##More Info
- [Usage Manual](docs/MANUAL.md)
- [Building from Source](docs/BUILDING.md)
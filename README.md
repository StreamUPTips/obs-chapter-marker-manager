# Overview​
The StreamUP Chapter Marker Manager is your all in one solution for creating and tracking chapter markers in your recordings in OBS. Since creating content on multiple platforms is so important nowadays, this will help you keep track of the moments you want to make videos out of.

There are many different ways you can create chapter markers and annotations:
- In the OBS UI via the docks.
- WebSocket. (This means it is compatible with software like Streamer.Bot)
- Hotkeys. (You can even set chapter names to use on different hotkeys!)
- Automatically. (This will automatically add chapter markers when you change scene. Don’t worry, you can set scenes to ignore!)

All of these chapter markers can be exported in various ways:
- Insert them into the actual video. (This will require OBS 30.2.0 and above. You will also need to use a compatible file type such as Hybrid mp4)
- Export them to a text file.
- Export them to an XML file.

# Guide​
If you want to find out more about the plugin and how to use it then please check out the [full guide](https://streamup.notion.site/OBS-Chapter-Marker-Manager-1567a7ed1438449cbbe8be87143568df?pvs=74)!

If you have any feedback or requests for this plugin please share them in the [StreamUP Discord server](https://discord.com/invite/RnDKRaVCEu?).

# Build
1. In-tree build
    - Build OBS Studio: https://obsproject.com/wiki/Install-Instructions
    - Check out this repository to plugins/streamup
    - Add `add_subdirectory(streamup)` to plugins/CMakeLists.txt
    - Rebuild OBS Studio

1. Stand-alone build (Linux only)
    - Verify that you have package with development files for OBS
    - Check out this repository and run `cmake -S . -B build -DBUILD_OUT_OF_TREE=On && cmake --build build`

# Support
This plugin is manually maintained by Andi as he updates all the links constantly to make your life easier. Please consider supporting to keep this plugin running!
- [**Patreon**](https://www.patreon.com/Andilippi) - Get access to all my products and more exclusive perks
- [**Ko-Fi**](https://ko-fi.com/andilippi) - Get access to all my products and more exclusive perks
- [**PayPal**](https://www.paypal.me/andilippi) - Send me a beer to say thanks!
- [**Twitch**](https://www.twitch.tv/andilippi) - Come say hi and feel free to ask questions!
- [**YouTube**](https://www.youtube.com/andilippi) - Learn more about OBS and upgrading your streams


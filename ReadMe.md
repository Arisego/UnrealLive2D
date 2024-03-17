# UnRealLive2D

Unreal Engine plugin for Live2D model.

![Screen Shot](Docs/Image/ScreenShot.GIF?raw=true)

The model logic part is based on [CubismNativeFramework](https://github.com/Live2D/CubismNativeFramework/). 

The render part take advantage of the [global shader plugin system](https://docs.unrealengine.com/en-US/Programming/Rendering/ShaderInPlugin/QuickStart/index.html). 

## Usage

1. Put this plugin in your project's Plugin folder
2. Download [Cubism Native SDK](https://www.live2d.com/en/download/cubism-sdk/download-native/) and copy `lib` and `include` to [CubismLibrary](https://github.com/Arisego/Live2DTest/tree/master/Plugins/UnrealLive2D/Source/ThirdParty/CubismLibrary) folder

> A sample of the plugin usage can be found [here](https://github.com/Arisego/Live2DTest).

You must donwload the corresponding version of [Cubism Native SDK](https://www.live2d.com/en/download/cubism-sdk/download-native/), or someting may not work as expected.

After plugin is compiled and installed, you can find *CubismMap* in plugin content folder. That's the sample map showing how model is rendered to RenderTarget and then displayed in UMG and World.

If you want to package game with the models in plugin folder, remeber to add `../Plugins/UnrealLive2D/Content/Samples` to your package copy path.

## Notice

This plugin was tested on __Win64__ envrioment only, it was supposed to work on all the platform that Cubism Native SDK supported, but some additional work might be needed. You can start with configuring `UnrealLive2D.Build.cs`.

Render result is only tested with *Hiyori* and *Mark*.

Some of the logic is not linked to blueprint and the control of the model logic is a bit dirty, this plugin is intend to be a good start for project that want to use Live2D model in Unreal Engine.

There will be **NO** Edtior part for this plugin.
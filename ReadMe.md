# UnRealLive2D

Unreal Engine plugin for Live2D model.

The model logic part is based on [CubismNativeFramework](https://github.com/Live2D/CubismNativeFramework/), some of the original comment is not removed to make it easier to work with SDK
version change.

The render part take advantage of the [global shader plugin system](https://docs.unrealengine.com/en-US/Programming/Rendering/ShaderInPlugin/QuickStart/index.html). 

## Usage

Put this plugin in your project's Plugin folder will work.

You must donwload the corresponding version of [Cubism Native SDK](https://www.live2d.com/en/download/cubism-sdk/download-native/), or someting may not work as expected.

After plugin is compiled and installed, you can find *CubismMap* in plugin content folder. That's the sample map showing how model is rendered to RenderTarget and then displayed in UMG and World.

If you want to package game with the models in plugin folder, remeber to add `../Plugins/UnrealLive2D/Content/Samples` to your package copy path.

A sample of the plugin usage can be found [here](https://github.com/Arisego/Live2DTest).

## Notice

This plugin should work on all the platform that Cubism Native SDK supported. If you want to work with platform other than __Win64__, you have to do some config work in `UnrealLive2D.Build.cs`.

Render result is only tested with *Hiyori* and *Mark*.

Some of the logic is not linked to blueprint and the control of the model logic is a bit dirty, this plugin is intend to be a good start for project that want to use Live2D model in Unreal Engine.

There will be **NO** Edtior part for this plugin.

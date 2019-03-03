# Software_D3D9

Performs software emulation for all of Direct3D9 via a proxy DLL (drop-in proxy replacement for Microsoft's official **d3d9.dll**). This project is intended to be a working full software emulation of Direct3D9 for educational purposes. It is intended to work with real games and other programs, and eventually should support a high level of compatibility with a wide range of existing D3D9 games and programs. Since this is software emulating hardware, this will never be as fast as a real GPU. Hopefully people can use this to learn more about how D3D9 works under the hood and to feel inspired about how 3D graphics works as a larger whole.

## Getting Started

Download the repo and open **Software_d3d9.sln** in Visual Studio 2017 or higher (or if you really want to you could hack the project and solution files to use an earlier version of Visual Studio).

### Prerequisites

You will need to have Visual Studio 2017 or higher installed, both for building this project, and for runtime JIT as well.
```
Get Visual Studio 2017 from Microsoft, it's free now for noncommercial use and it's still arguably the best IDE on Windows for C++ programming!
```

You will also need to have the DirectX9 SDK installed on your computer. The latest version of this is the "June 2010" DirectX SDK available here (https://www.microsoft.com/en-us/download/details.aspx?id=6812). This is currently needed for access to D3DX9, which will eventually be removed as a requirement in the future.

After installing the DirectX SDK, you need to add the Include and Lib paths to Visual Studio so that it knows where to look for these new **.h** header and **.lib** library files.
```
How this is done differs between versions of Visual Studio, but the way to do it in VS2017 is:
Go to the Property Manager (note that this is different than the Property Window), and under the Debug|Win32 or Release|Win32 folders, right-click and go to Properties for the Microsoft.Cpp.Win32.user property sheet.
Now go to VC++ Directories in the left hand tree view.
For Include Directories, add the entry that corresponds to the Includes directory where you just installed the DirectX SDK. By default that path will be: C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include
For Library Directories, add the entry that corresponds to the Lib directory where you just installed the DirectX SDK. Make sure to pick the right processor architecture that matches the current build configuration (so for a Win32 build configuration, you should select the x86 directory, and for a Win64 build configuration you should select the x64 directory). By default that path will be one of:
For x86: C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Lib\x86
For x64: C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Lib\x64
Now click OK at the bottom to save your changes, and you should be good to go!
```

### Building

Open the **Software_d3d9.sln** solution in Visual Studio 2017 (or higher).
Once the project and all associated files have finished loading/initializing, select the desired Solution Configuration for your target program.
```
Currently supported targets are:
Debug - Win32
Release - Win32

Future configuration targets will support Win64 as well.
```

If you run into any compilation errors with the C/C++ code, they're likely due to the language conformance setting in Visual Studio. This project was written without using many modern C/C++ features (basically the C++11 featureset), but also not necessarily conforming to any future strictness that may be added into the C++ standard.

If you experience missing includes for **d3dx9.h**, then make sure that you have the DirectX SDK installed (see above section) and that you have properly added the DirectX SDK's Include folder to your Include Directories.
```
By default, this path should be: C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include
```

If you run into any compilation errors with the HLSL code, that's likely due to shader model or shader type being set incorrectly. Make sure that the Shader Model is set to Shader Model 2 or Shader Model 3.
```
For Pixel Shaders, this should be either "ps_2_0" or "ps_3_0".
For Vertex Shaders, this should be either "vs_2_0" or "vs_3_0".
It's probably a good idea to keep all of the shaders built in this project on the same version if possible.
```

If you run into any linker errors, check and make sure that all of the **.c** and **.cpp** files are properly added to the project and that they are properly marked as "C/C++ compiler" under the "Item Type" property for each file so that they properly participate in the build.

If you have linker errors relating to **d3dx9.lib**, then ensure that you have properly added the DirectX SDK's Lib folder to your Include Directories.
```
By default, this path should be:
For x86: C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Lib\x86
For x64: C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Lib\x64
```

## Deployment

After building, go to the appropriate build output files folder depending on your build configuration:
```
Debug/Win32: .\Debug\
Release/Win32: .\Release\
Debug/Win64: .\x64\Debug\
Release/Win64: .\x64\Release\
```
Copy the newly build **d3d9.dll** file from that folder and paste it into the directory next to the program that you wish to test with it. Depending on how this program is launched and how it is configured to load DLLs, you may want to instead place the proxy DLL into the working directory of your test program rather than the immediate directory in which the executable resides. Due to how DLL loading works on Windows (unless programs specify otherwise), the proxy **d3d9.dll** should be picked up rather than the true Microsoft **d3d9.dll**.

If you run into errors while trying to launch your program about a missing **d3dx9_43.dll** (or any other version of "d3dx9_XX.dll"), then be sure to install the DirectX End-User Runtimes (https://www.microsoft.com/en-us/download/details.aspx?id=8109) before trying to launch your program. This should not be a problem on Windows 8 and up as these DLLs ship with the operating system, but may be an issue on Windows XP, Windows Vista, or Windows 7.

## Built With

* [Wrappit] (https://www.codeproject.com/Articles/16541/Create-your-Proxy-DLLs-automatically) - Used to generate the proxy DLL template (which has since been very heavily modified).
* [D3DX9] (https://www.microsoft.com/en-us/download/details.aspx?id=6812) - Used as a vector maths library (D3DXMath) and for some debug functionality (D3DXSaveSurfaceToFile(), for example, is useful for debugging surfaces dumped from memory to disk). In the future I plan to remove this as a requirement and replace D3DXMath with XNAMath.

## Authors

* **Tom Lopes** - *Initial work* - [code-tom-code](https://github.com/code-tom-code)

See also the list of [contributors](https://github.com/code-tom-code/Software_D3D9/contributors) who participated in this project.

## License

This project is licensed under the zLib/LibPNG License - see the [LICENSE.txt](LICENSE.txt) file for details

## Acknowledgments

* Thanks to Michael Chourdakis for his Wrappit code (https://www.codeproject.com/Articles/16541/Create-your-Proxy-DLLs-automatically). It's been useful for me for years now for easily generating a starting-point for a proxy DLL.
* Thanks to Microsoft and MSDN for having awesome documentation for the Direct3D9/9Ex and the HLSL shader bytecode format docs to go off of. They have really helped with navigating edge cases and solving tricky problems.
* Thanks to rygorous (Fabian 'ryg' Giesen) for authoring his series of articles that illuminate how modern GPUs work under the hood in great detail (https://fgiesen.wordpress.com/2011/07/09/a-trip-through-the-graphics-pipeline-2011-index/).

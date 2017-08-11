# F4DConverter
This application, F4DConverter, is for converting popular 3D model formats into F4D format
which is devised for Mago3D - 3D web geo-platform. (www.mago3d.com).
This project is of Microsoft Visual Studio 2015 C++ project.

## developer's comments before reading more ##
- F4DConverter runs only in Windows 7 or later version of 64-bit OS.
- This is window-based BUT FAKES as it runs in CLI mode. So you have to run this in cmd.exe.
- Currently, this has bugs in converting .3ds, specially in making SimpleBuildingTexture3x3.png and applying transformation matrix on some data nodes.

## supported input formats ##
- .ifc
- .3ds
- .obj
- .dae

> Beside above formats, other formats which are supported by Assimp may be supported.(NOT TESTED!!)
>
> In this version, .JT(Jupiter Tessellation, a kind of cad design format) is not included.

## necessary libraries for F4DConverter ##
- OpenSceneGraph 3.4.0 : http://www.openscenegraph.org
- ifcplusplus : https://github.com/ifcquery/ifcplusplus
- Carve : https://github.com/ifcquery/ifcplusplus
- Assimp 3.2 : http://assimp.sourceforge.net/main_downloads.html
- boost 1.62 : http://www.boost.org/users/history/version_1_62_0.html
- SOIL : http://www.lonesock.net/soil.html
- glew 2.0 : http://glew.sourceforge.net/

> ifcplusplus, Assimp, SOIL, and glew are for F4DConverter directly.
>
> Carve, boost, and OpenSceneGraph are for ifcplusplus.

## how to use ##
### sample arguments ###
- -inputFolder D:/data/data_3ds/DC_library_del_3DS -outputFolder D:/data/conversionResult -log D:/data/conversionResult/logTest.txt -idPrefix design_ -idSuffix _2017 -oc y -usf 0.01
- -outputFolder D:/dataConverted -indexing y
### explanation of each argument ###
- -inputFolder [rawDataFolder] : an absolute path of the folder where raw data files to be converted are.
- -outputFolder [F4DFolder] : an absolute path of the folder where conversion results(F4D datasets) created.

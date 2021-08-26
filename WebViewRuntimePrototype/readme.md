Title
===
WebView2 Boostrapper prerequisite prototype

# Description
This is a set of files that prototypes the usage of a WebView2 prerequisite component to be shipped as part of Visual Studio in a later release.  This can then be used
by vistual studio click once projects to find the boostrapper component sources.

# Whats in this
The files should be:

| File | Description |
| ------------- | -------------------------------------------------------- |
| \product.xml | A descriptive block covering what the prerequisite is for |
| \en\package.xml | Block containing file information for the eula |
| \en\placeholdereula.txt | A placeholder eula for this prototype |
| | |
| \prerequisiteprototype.reg | a sample reg file for configuration |
| \readme.md | this readme file |

# Installation
Put the files on your development machine under a bootstrapper\prerequisite folder.  
IE:
e:\boostreapper\prerequisite\webviewruntimeprototype

Create a reg key named:
HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\GenericBootstrapper\AdditionalPackagePaths

With a REG_SZ key under it:
WebView2Runtime  value = the path you used above (minus the webviewruntimeprototype portion)
IE: 
E:\boostrapper\prerequisite


Now when the click once project presents the list of prerequisites available the webviewruntime will show up.



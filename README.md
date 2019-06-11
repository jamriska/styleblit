# StyleBlit: Fast Example-Based Stylization with Local Guidance
_[D.Sýkora](https://dcgi.fel.cvut.cz/home/sykorad/), [O. Jamriška](https://dcgi.fel.cvut.cz/people/jamriond), [O. Texler](https://ondrejtexler.github.io/), [J. Fišer](https://research.adobe.com/person/jakub-fiser/), [M. Lukáč](https://research.adobe.com/person/michal-lukac/), [J. Lu](https://research.adobe.com/person/jingwan-lu/), and [E. Shechtman](https://research.adobe.com/person/eli-shechtman/)_

[[`WebPage`](https://dcgi.fel.cvut.cz/home/sykorad/styleblit.html)], [[`Paper`](https://dcgi.fel.cvut.cz/home/sykorad/Sykora19-EG.pdf)], [[`Slides`](https://dcgi.fel.cvut.cz/home/sykorad/Sykora19-EG.pptx)], [[`BiBTeX`](#CitingStyleBlit)], [[`Unity3D Asset`](https://dcgi.fel.cvut.cz/home/sykorad/StyleBlit/unity_plugin.zip)]

## Short Abstract
StyleBlit is an efficient example-based style transfer algorithm that can deliver high-quality stylized renderings in real-time on a single-core CPU. Our technique is especially suitable for style transfer applications that use local guidance - descriptive guiding channels containing large spatial variations. Local guidance encourages transfer of content from the source exemplar to the target image in a semantically meaningful way. Typical local guidance includes, e.g., normal values, texture coordinates or a displacement field. Contrary to previous style transfer techniques, our approach does not involve any computationally expensive optimization. 

## StyleBlit live-web-demo

<!-- [![Live-web-demo](styleblitlive.gif)](https://dcgi.fel.cvut.cz/home/sykorad/StyleBlit/demo.html) -->

<p align="center">
  <a href="https://dcgi.fel.cvut.cz/home/sykorad/StyleBlit/demo.html"><img src="styleblitlive.gif" width="600px" /> </a>
</p>


## Build / Installation
Like the demo above? ... try it by yourself!

This implementation is written in C++ using OpenGL3.2. Build scripts for web-app, MacOS desktop app, and Windows desktop app are provided.

### Build StyleBlit as Web application
* We use Emscripten SDK to build the web app - to setup the Emscripten SDK, follow the instructions here: https://emscripten.org/docs/getting_started/downloads.html
* Once Emscripten SDK is configured (i.e., em++ command is found on your PATH and works), run `build-emscripten.bat`
* Run `styleblit.html` in your browser
* Possible issue: Google Chrome for security reasons might refuse to run html with emscripten code without http (file:///C:/.../styleblit.html), use different web browser or run it using HTTP server. For example `python -m http.server 8000` (http://<span></span>localhost:8000/styleblit.html)    


### Build StyleBlit for Windows
* Make sure you have Visual Studio installed (cl.exe compiler)
* Run `build-windows.bat`
* Run `styleblit.exe`


### Build StyleBlit for MacOS
* Make sure you have clang compiler
* Run `build-macos.sh`
* Run `styleblitapp`


## <a name="CitingStyleBlit"></a>Citing StyleBlit
If you find StyBlit usefull for your research or work, please use the following BibTeX entry.

```
@Article{Sykora19-EG,
  author =  "Daniel S\'{y}kora and Ond\v{r}ej Jamri\v{s}ka and Ond\v{r}ej Texler 
             and Jakub Fi\v{s}er and Michal Luk\'{a}\v{c} and Jingwan Lu and Eli Shechtman",
  title =   "{StyleBlit}: Fast Example-Based Stylization with Local Guidance",
  journal = "Computer Graphics Forum",
  volume =  38,
  number =  2,
  pages =   "83--91",
  year =    2019,
}
```

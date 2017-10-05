
<a href="https://mapbox.github.io/mvt-cruncher/html/mvt2svg.html">DEMO</a>

## Web browser based 'drag-n-drop' MVT to SVG converter



### Requirements

* emscripten SDK (https://kripken.github.io/emscripten-site/docs/getting_started/downloads.html)
* boost >= 1.64 (http://www.boost.org/)
* variant (https://github.com/mapbox/variant)
* geometry.hpp (https://github.com/mapbox/geometry.hpp)
* protozero (https://github.com/mapbox/protozero)
* vtzero (https://github.com/mapbox/vtzero)


### Building

Edit `Jamroot` to match your setup and then :

`b2 toolset=emscripten release`

### Running

* Copy `*.js` and `*.wasm` files into `html` directory
* Start web server `busybox httpd -f -p 8000`
* Open `http://localhost:8000/html/mvt2svg.html` in a browser

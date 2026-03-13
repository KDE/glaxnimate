Glaxnimate Web
==============

Web library to embed and edit Glaxnimate animation on web pages.


Usage
-----

### Loading the library

You can load from CDN. Both jsdelivr and unpkg are supported:

```html
<script src="https://cdn.jsdelivr.net/npm/glaxnimate/glaxnimate.min.js"></script>
```

or

```html
<script src="https://unpkg.com/glaxnimate@latest/glaxnimate.js"></script>
```

### Usage

```html
<script>
// Create the player  
let player = new Glaxnimate.Player({
    canvas: document.getElementById("canvas")
});

// Load an animation explicitly
// You can also just pass these to the constructor if data is available
player.load({
    // `data` can be a string or Uint8Array
    data: "your data", 
    // You have to specify `format` or `filename` so the correct importer can be used
    format: "glaxnimate"
});

// Fetch a remote animation
player.fetch("example.rawr");

// ... 

// Control playback
player.pause();
player.play();

// Edit the animation live
let fill = player.composition.shapes[0].shapes[0].shapes[0];
fill.color.value = {red: 255, green: 0, blue: 255, alpha: 255};
</script>
```

Supported Formats
-----------------

* Glaxnimate `.rawr`
* Lottie `.lot`, `.json`
* SVG `.svg` 
* AEP `.aep`, `.aepx`

Experimental formats:

* Andorid Vector Drawables `.avd`
* Rive `.riv`

Links
-----

* [Repo](https://invent.kde.org/graphics/glaxnimate)
* [Issues](https://bugs.kde.org/describecomponents.cgi?product=glaxnimate)
* [npm](https://www.npmjs.com/package/glaxnimate)
* CDN [unpkg](https://app.unpkg.com/glaxnimate) [jsdelivr](https://www.jsdelivr.com/package/npm/glaxnimate)

License
-------

GPLv3+


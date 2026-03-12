Glaxnimate Web
==============

Web library to embed and edit Glaxnimate animation on web pages.


Usage
-----


You can load from CDN

```html
<script src="https://unpkg.com/glaxnimate@latest/glaxnimate.js"></script>
<script>

// Create the player    
let player = new Glaxnimate.Player({
    canvas: document.getElementById("canvas")
});

// Load a remote animation 
// You can also just pass these to the constructor if data is available
fetch("example.rawr")
.then(r => r.text())
.then(data => player.load({
    data,
    format: "glaxnimate"
}));

// ... 

// Control playback
player.pause();
player.play();

// Edit the animation live
let fill = player.composition.shapes[0].shapes[0].shapes[0];
fill.color.value = {red: 255, green: 0, blue: 255, alpha: 255};

</script>
```

Links
-----

* [Repo](https://invent.kde.org/graphics/glaxnimate)
* [Issues](https://bugs.kde.org/describecomponents.cgi?product=glaxnimate)
* [npm](https://www.npmjs.com/package/glaxnimate)
* [CDN](https://app.unpkg.com/glaxnimate)

License
-------

GPLv3+


/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 * SPDX-License-Identifier: BSD-2-Clause
 */
(() => {
    Glaxnimate.load_actions = [];
    Glaxnimate().then((Module) => {
        Glaxnimate.Module = Module;
        Module.initialize();

        for ( let [k, v] of Object.entries(Module) )
            if ( !k.match(/^[A-Z0-9]+$|^[Qq]t|^promise/) )
                Glaxnimate[k] = v;

        for ( let action of Glaxnimate.load_actions )
            action(Module);
        delete Glaxnimate.load_actions;
    });
    Glaxnimate.load_action = func => {
        if ( Glaxnimate.Module )
            func(Glaxnimate.Module);
        else
            Glaxnimate.load_actions.push(func);
    };

    class Player
    {
        constructor(opts)
        {
            this._play = opts.autoplay ?? true;
            this._opts = opts;
            this.canvas = opts.canvas;
            this.context = this.canvas.getContext("2d");
            this._start_time = -1;
            this._frames_offset = 0
            Glaxnimate.load_action(this.load.bind(this));
        }

        load()
        {
            this.renderer = new Glaxnimate.GlaxnimateRenderer(this._opts);
            if ( this._play )
                requestAnimationFrame(this.draw_frame.bind(this));
        }

        play()
        {
            if ( !this._play )
            {
                this._play = true;
                this._start_time = -1;
                requestAnimationFrame(this.draw_frame.bind(this));
            }
        }

        pause()
        {
            if ( this._play )
            {
                this._play = false;
                this._frames_offset = this.renderer.current_time;
            }
        }

        draw_frame(time)
        {
            if ( this._start_time < 0 )
                this._start_time = time;

            let time_frames = ((time - this._start_time) / 1000 * this.renderer.fps + this._frames_offset) % this.renderer.last_frame;
            this.renderer.current_time = time_frames;
            const pixels = this.renderer.render();
            this.canvas.width = this.renderer.width;
            this.canvas.height = this.renderer.height;
            const img = this.context.createImageData(this.renderer.width, this.renderer.height);
            img.data.set(pixels);
            this.context.putImageData(img, 0, 0);
            if ( this._play )
                requestAnimationFrame(this.draw_frame.bind(this));
        }
    }

    Glaxnimate.Player = Player;
})();

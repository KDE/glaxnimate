/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
(() => {
    Glaxnimate.load_actions = [];
    Glaxnimate.initialized = false;
    Glaxnimate().then((Module) => {
        Glaxnimate.Module = Module;
        Module.initialize();

        for ( let [k, v] of Object.entries(Module) )
            if ( !k.match(/^[A-Z0-9]+$|^[Qq]t|^promise/) )
                Glaxnimate[k] = v;

        Glaxnimate.initialized = true;

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
            this._play = false;
            this._opts = opts;
            this.canvas = opts.canvas;
            this.context = this.canvas.getContext("2d");
            this._start_time = -1;
            this._frames_offset = 0
            this._animation_frame = null;
            Glaxnimate.load_action(() => this.load());
        }

        _update_opts(opts)
        {
            if ( opts )
            {
                this._opts = {...this._opts, ...opts};
            }
        }

        fetch(url, opts=undefined)
        {
            this._update_opts(opts);
            fetch(url)
            .then(r => r.bytes())
            .then(data => player.load({
                data,
                filename: url
            }));
        }

        load(opts=undefined)
        {
            this._update_opts(opts);

            if ( !Glaxnimate.initialized || !this._opts.data )
                return false;

            this.pause();
            this.renderer = new Glaxnimate.GlaxnimateRenderer(this._opts);

            // Clear loader-specific options
            delete this._opts.data;
            delete this._opts.filename;
            delete this._opts.format;

            // Loading failed
            if ( !this.renderer.composition )
            {
                this.context.clearRect(0, 0, this.canvas.width, this.canvas.height);
                return false;
            }

            if ( this._opts.autoplay ?? true )
                this.play();

            return true;
        }

        play()
        {
            if ( !this._play )
            {
                this._play = true;
                this._start_time = -1;
                this._request_frame();
            }
        }

        pause()
        {
            if ( this._play )
            {
                this._play = false;
                this._frames_offset = this.renderer.current_time;
                if ( this._animation_frame )
                {
                    cancelAnimationFrame(this._animation_frame);
                    this._animation_frame = null;
                }
            }
        }

        render_frame(time_frames)
        {
            this.renderer.current_time = time_frames;
            const pixels = this.renderer.render();
            this.canvas.width = this.renderer.width;
            this.canvas.height = this.renderer.height;
            const img = this.context.createImageData(this.renderer.width, this.renderer.height);
            img.data.set(pixels);
            this.context.putImageData(img, 0, 0);
        }

        _request_frame()
        {
            this._animation_frame = requestAnimationFrame(this._render_tick.bind(this));
        }

        _render_tick(time)
        {
            this._animation_frame = null;
            if ( this._start_time < 0 )
                this._start_time = time;

            let time_frames = ((time - this._start_time) / 1000 * this.renderer.fps + this._frames_offset) % this.renderer.last_frame;

            this.render_frame(time_frames);

            if ( this._play )
                this._request_frame();
        }

        get document()
        {
            return this.renderer.document;
        }

        get composition()
        {
            return this.renderer.composition;
        }
    }

    Glaxnimate.Player = Player;
})();

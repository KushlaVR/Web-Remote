class WorkSpace {


    private form: HTMLFormElement;

    socket: WebSocket;

    private inputs: Array<Input> = new Array<Input>();
    private outputs: Array<Output> = new Array<Input>();
    values: Dictionary<string> = new Dictionary<string>();
    tranCount: number = 0;

    constructor(form: HTMLFormElement) {
        this.form = form;
        window.addEventListener('resize', (event: any) => this.UpdateLayout(), false);
    }

    public static init(form: JQuery) {
        var workSpace: WorkSpace = new WorkSpace(<any>(form[0]));
        workSpace.registerInputs();
        workSpace.registerOutputs();
        workSpace.Connect();
    }


    private Connect(): void {
        var jqxhr = $.get("/api/pipename")
            .done((pipename) => {
                this.socket = new WebSocket(pipename);
                this.socket.onmessage = (msg: any) => {
                    $("#message").text(msg);
                };

                this.socket.onclose = (event: any) => {
                    alert('Disconnected');
                };
            })
            .fail(function () {
                console.log("error");
            });
    }

    static toggleFullScreen(): void {
        var doc: any = window.document;
        var docEl: any = doc.documentElement;

        var requestFullScreen = docEl.requestFullscreen || docEl.mozRequestFullScreen || docEl.webkitRequestFullScreen || docEl.msRequestFullscreen;
        var cancelFullScreen = doc.exitFullscreen || doc.mozCancelFullScreen || doc.webkitExitFullscreen || doc.msExitFullscreen;

        if (!doc.fullscreenElement && !doc.mozFullScreenElement && !doc.webkitFullscreenElement && !doc.msFullscreenElement) {
            requestFullScreen.call(docEl);
        }
        else {
            cancelFullScreen.call(doc);
        }
    }

    UpdateLayout() {
        for (var i: number = 0; i < this.inputs.length; i++) {
            this.inputs[i].initLayout();
        }
        for (var o: number = 0; i < this.outputs.length; i++) {
            this.outputs[o].initLayout();
        }
    }

    private registerInputs() {
        var inputs = $(".input", this.form);

        inputs.each((index: number, val: any) => {
            let element: HTMLElement = val;
            var input: Input;
            if ($(element).hasClass("slider")) {
                var slider: Slider = new Slider(element);
                input = slider;
            } else {
                input = new Input(element);
            }
            this.addInput(input);
        })
    }

    private addInput(input: Input): void {
        input.workSpace = this;
        this.inputs.push(input);
        input.refreshValues();
    }

    private registerOutputs() {
        var outputs = $(".output", this.form);

        outputs.each((index: number, val: any) => {
            let element: HTMLElement = val;
            var output: Output;
            output = new Output(element);
            this.addOutput(output);
        })
    }

    private addOutput(output: Output): void {
        output.workSpace = this;
        this.outputs.push(output);
        output.refreshValues();
    }


    beginTransaction() {
        this.tranCount += 1;
    }

    endTransaction() {
        this.tranCount -= 1;
        if (this.tranCount === 0) {
            for (var i: number = 0; i < this.outputs.length; i++) {
                this.outputs[i].refreshValues();
            }
            if (this.socket) this.socket.send(JSON.stringif(this.values));
        }
    }


}

interface IDictionary<T> {
    add(key: string, value: T): void;
    remove(key: string): void;
    containsKey(key: string): boolean;
    keys(): string[];
}

class Dictionary<T> implements IDictionary<T> {

    _keys: string[] = [];

    constructor(init?: { key: string; value: T; }[]) {
        if (init) {
            for (var x = 0; x < init.length; x++) {
                this[init[x].key] = init[x].value;
                this._keys.push(init[x].key);
            }
        }
    }

    add(key: string, value: T) {
        this[key] = value;
        this._keys.push(key);
    }

    remove(key: string) {
        var index = this._keys.indexOf(key, 0);
        this._keys.splice(index, 1);
        delete this[key];
    }

    keys(): string[] {
        return this._keys;
    }

    containsKey(key: string) {
        if (typeof this[key] === "undefined") {
            return false;
        }
        return true;
    }

    toLookup(): IDictionary<T> {
        return this;
    }
}

class Point {
    x: number = 0;
    y: number = 0;
}

class Input {

    workSpace: WorkSpace;
    element: HTMLElement;
    jElement: JQuery;
    name: string;

    constructor(element: any) {
        this.element = element;
        this.jElement = $(element);
        this.name = this.jElement.attr("name");
    }

    refreshValues(): void {
        if (!this.workSpace) return;
        this.workSpace.beginTransaction();
        let val: string = this.jElement.attr("value")
        if (val) {
            if (!this.workSpace.values.containsKey(this.name)) {
                this.workSpace.values.add(this.name, val)
            } else {
                this.workSpace.values[this.name] = val;
            }
        }
        this.workSpace.endTransaction();
    }

    initLayout(): void {

    }
}

class Slider extends Input {

    private handle: HTMLElement;
    private pot: HTMLElement;

    private pressed: boolean = false;

    private handlePos: Point = new Point();
    private value: Point = new Point();
    private center: Point = new Point();

    public autoCenterX: boolean = false;
    public autoCenterY: boolean = false;

    constructor(element: any) {
        super(element);
        this.handle = $(".handle", element)[0];
        var pot = $(".pot", element)
        if (pot.length > 0) {
            this.pot = pot[0];
        }

        if ("ontouchstart" in document.documentElement) {
            this.element.addEventListener('touchstart', (event: any) => this.onTouchStart(event), false);
            this.element.addEventListener('touchmove', (event: any) => this.onTouchMove(event), false);
            this.element.addEventListener('touchend', (event: any) => this.onTouchEnd(event), false);
        }
        else {
            this.element.addEventListener('mousedown', (event: any) => this.onMouseDown(event), false);
            this.element.addEventListener('mousemove', (event: any) => this.onMouseMove(event), false);
            this.element.addEventListener('mouseup', (event: any) => this.onMouseUp(event), false);
        }

        this.initLayout();

        if ($(element).data("center")) {
            this.autoCenterX = true;
            this.autoCenterY = true;
        } else if ($(element).data("center-x")) {
            this.autoCenterX = true;
        } else if ($(element).data("center-y")) {
            this.autoCenterY = true;
        }

        this.refreshLayout(true);
    }

    private onTouchStart(event): void {
        this.pressed = true;
        this.element.style.zIndex = "100";
    }
    private onTouchMove(event: TouchEvent) {
        // Prevent the browser from doing its default thing (scroll, zoom)
        event.preventDefault();
        if (this.pressed === true) {
            this.handlePos = Slider.pointFromTouch(this.element, event.targetTouches[0])
            this.refreshLayout(false);
            this.refreshValues();
        }
    }
    private onTouchEnd(event): void {
        this.pressed = false;
        // If required reset position store variable
        if (this.autoCenterX)
            this.handlePos.x = this.center.x;
        if (this.autoCenterY)
            this.handlePos.y = this.center.y;
        this.refreshLayout(true);
        this.refreshValues();
        this.element.style.zIndex = "0";
    }

    private onMouseDown(event): void {
        this.pressed = true;
        this.element.style.zIndex = "100";
    }
    private onMouseMove(event): void {
        if (this.pressed === true /*&& event.target === this.element*/) {
            this.handlePos = Slider.pointFromMouseEvent(this.element, event);
            this.refreshLayout(false);
            this.refreshValues();
        }
    }
    private onMouseUp(event): void {
        this.pressed = false;
        // If required reset position store variable
        if (this.autoCenterX)
            this.handlePos.x = this.center.x;
        if (this.autoCenterY)
            this.handlePos.y = this.center.y;
        this.refreshLayout(true);
        this.refreshValues();
        this.element.style.zIndex = "0";
    }

    private refreshLayout(clip: boolean): void {
        if (clip) {
            if (this.handlePos.x < 0) this.handlePos.x = 0;
            if (this.handlePos.y < 0) this.handlePos.y = 0;
            if (this.handlePos.x > this.element.clientWidth) this.handlePos.x = this.element.clientWidth;
            if (this.handlePos.y > this.element.clientHeight) this.handlePos.y = this.element.clientHeight;
        }

        this.handle.style.left = '' + (this.handlePos.x - (this.handle.clientWidth / 2)) + 'px';
        this.handle.style.top = '' + (this.handlePos.y - (this.handle.clientHeight / 2)) + 'px';


        var clipped: Point = new Point();
        clipped.x = this.handlePos.x;
        clipped.y = this.handlePos.y;
        if (clipped.x < 0) clipped.x = 0;
        if (clipped.y < 0) clipped.y = 0;
        if (clipped.x > this.element.clientWidth) clipped.x = this.element.clientWidth;
        if (clipped.y > this.element.clientHeight) clipped.y = this.element.clientHeight;

        let normalized: Point = new Point();
        normalized.x = (this.center.x - clipped.x) * 100.0 / (this.element.clientWidth / 2.0);
        normalized.y = (this.center.y - clipped.y) * 100.0 / (this.element.clientHeight / 2.0);

        this.value = normalized;

        if (this.pot) {
            this.pot.style.left = '' + (clipped.x - (this.pot.clientWidth / 2.0)) + 'px';
            this.pot.style.top = '' + (clipped.y - (this.pot.clientHeight / 2.0)) + 'px';
        }
    }

    refreshValues(): void {
        if (!this.workSpace) return;
        this.workSpace.beginTransaction();
        let key_x = this.name + "_x";
        if (!this.workSpace.values.containsKey(key_x)) {
            this.workSpace.values.add(key_x, Slider.numToString(this.value.x))
        } else {
            this.workSpace.values[key_x] = Slider.numToString(this.value.x);
        }

        let key_y = this.name + "_y";
        if (!this.workSpace.values.containsKey(key_y)) {
            this.workSpace.values.add(key_y, Slider.numToString(this.value.y))
        } else {
            this.workSpace.values[key_y] = Slider.numToString(this.value.y);
        }
        this.workSpace.endTransaction();
    }

    initLayout(): void {

        this.center.x = this.element.clientWidth / 2;
        this.center.y = this.element.clientHeight / 2;

        let x = this.element.clientWidth / 2.0;
        let y = this.element.clientHeight / 2.0;

        this.handlePos.x = this.center.x - this.value.x * x / 100.0;
        this.handlePos.y = this.center.y - this.value.y * y / 100.0;

        this.handle.style.left = '' + (this.handlePos.x - (this.handle.clientWidth / 2)) + 'px';
        this.handle.style.top = '' + (this.handlePos.y - (this.handle.clientHeight / 2)) + 'px';

        if (this.pot) {
            this.pot.style.left = '' + (this.handlePos.x - (this.pot.clientWidth / 2.0)) + 'px';
            this.pot.style.top = '' + (this.handlePos.y - (this.pot.clientHeight / 2.0)) + 'px';
        }

    }

    private static numToString(n: number): string {
        return (Math.round(n * 100.0) / 100.0).toString(10);
    }

    private static pointFromMouseEvent(container: HTMLElement, e: any): Point {
        var m_posx = 0, m_posy = 0, e_posx = 0, e_posy = 0;
        //get mouse position on document crossbrowser
        if (!e) { e = window.event; }
        if (e.pageX || e.pageY) {
            m_posx = e.pageX;
            m_posy = e.pageY;
        } else if (e.clientX || e.clientY) {
            m_posx = e.clientX + document.body.scrollLeft
                + document.documentElement.scrollLeft;
            m_posy = e.clientY + document.body.scrollTop
                + document.documentElement.scrollTop;
        }
        //get parent element position in document
        if (container.offsetParent) {
            do {
                e_posx += container.offsetLeft;
                e_posy += container.offsetTop;
            } while (container = <any>container.offsetParent);
        }
        // mouse position minus elm position is mouseposition relative to element:
        var pt = new Point();
        pt.x = (m_posx - e_posx);
        pt.y = (m_posy - e_posy);
        return pt;

    }

    private static pointFromTouch(container: HTMLElement, e: Touch): Point {

        var m_posx = 0, m_posy = 0, e_posx = 0, e_posy = 0;
        //get mouse position on document crossbrowser
        //if (!e) { e = window.event; }
        if (e.pageX || e.pageY) {
            m_posx = e.pageX;
            m_posy = e.pageY;
        } else if (e.clientX || e.clientY) {
            m_posx = e.clientX + document.body.scrollLeft
                + document.documentElement.scrollLeft;
            m_posy = e.clientY + document.body.scrollTop
                + document.documentElement.scrollTop;
        }
        //get parent element position in document
        if (container.offsetParent) {
            do {
                e_posx += container.offsetLeft;
                e_posy += container.offsetTop;
            } while (container = <any>container.offsetParent);
        }
        // mouse position minus elm position is mouseposition relative to element:
        var pt = new Point();
        pt.x = (m_posx - e_posx);
        pt.y = (m_posy - e_posy);
        return pt;

    }

}

class Output {
    workSpace: WorkSpace;
    element: HTMLElement;
    jElement: JQuery;
    name: string;

    constructor(element: any) {
        this.element = element;
        this.jElement = $(element);
        this.name = this.jElement.data("input");
    }

    refreshValues(): void {
        if (this.workSpace.values.containsKey(this.name)) {
            if (this.element.tagName.toUpperCase() == "INPUT") {
                this.jElement.val(this.workSpace.values[this.name]);
            } else {
                this.jElement.text(this.workSpace.values[this.name]);
            }
        }
    }

    initLayout(): void {

    }
}



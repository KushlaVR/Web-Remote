/**
 * 
 * */
class WorkSpace {

    private form: HTMLFormElement;

    client: string;
    //socket: WebSocket;
    eventSource: EventSource;

    private inputs: Array<Input> = new Array<Input>();
    private outputs: Array<Output> = new Array<Output>();
    values: Dictionary<string> = new Dictionary<string>();
    sent: Dictionary<string> = new Dictionary<string>();
    tranCount: number = 0;
    timer: number = 0;
    reportInterval: number = 100;//Інтервал синхронізації даних
    fields: Array<string>;
    readonlyFields: Array<string> = new Array<string>();
    tran: number = 0;

    constructor(form: HTMLFormElement) {
        this.form = form;
        window.addEventListener('resize', (event: any) => this.UpdateLayout(), false);


    }

    public static init(form: JQuery) {
        var workSpace: WorkSpace = new WorkSpace(<any>(form[0]));
        workSpace.registerInputs();
        workSpace.registerOutputs();
        //workSpace.ConnectWS();
        workSpace.ConnectAPI();
    }

    private ConnectAPI(): void {
        $.get("/api/EventSourceName")
            .done((EventSourceName) => {
                if (window.EventSource) {
                    var s: string = EventSourceName;
                    var json: string = decodeURI(s.substring(s.indexOf("?") + 1)).replace("%3a", ":");
                    var parcel: any = JSON.parse(json);
                    this.client = parcel.client;
                    this.eventSource = new EventSource(EventSourceName);
                    this.eventSource.onopen = (ev: Event) => {

                        this.setFormat();
                        this.sendData();
                    }
                    this.eventSource.onmessage = (msg: MessageEvent) => {
                        $("#message").text(msg.data);
                        this.receiveData(msg);
                    };
                    this.eventSource.onerror = (event: Event) => {
                        $("#message").text("Error...");
                    };
                }
            })
            .fail(() => {
                $("#message").text("error");
            });
    }

    /** 
     *  Повідомляємо серверу в якій послідовності розміщено значення елементів керування
     */
    private setFormat(): void {
        this.fields = new Array();
        $.each(<any>(this.values), (name: string, value: string) => {
            this.fields.push(name);
        });

        this.send(JSON.stringify({ client: this.client, fields: this.fields, readonlyFields: this.readonlyFields }));
    }

    private _readyToSend: boolean = true;
    private readyToSend(): boolean {
        if (this.eventSource)
            return this._readyToSend;
        return false;
    }

    private send(value: string): void {
        if (this.eventSource) {
            this._readyToSend = false;
            $.ajax({
                url: "/api/post",
                data: value,
                cache: false,
                type: 'POST',
                dataType: "json",
                contentType: 'application/json; charset=utf-8'
            }).done(() => {
                console.log("done!");
                this._readyToSend = true;
            }).fail(() => {
                console.log("fail!");
                this._readyToSend = true;
            });
        } //else
        //if (this.socket) {
        //    this._readyToSend = false;
        //    this.socket.send(value);
        //    this._readyToSend = true;
        //}
    }

    /**
     * Запускаємо механізм відправки повідомлень
     * */
    private sendData(): void {
        //Якщо таймер не заведено, відправляємо пакет і запускаємо таймер
        if (this.timer === 0) {
            if (this.readyToSend() == true) {
                var v = new Array<string>();
                var changed: boolean = false;
                for (var i: number = 0; i < this.fields.length; i++) {
                    var key: string = this.fields[i];
                    var value = this.values[key];
                    v.push(value);
                    if (this.sent[key] !== this.values[key]) changed = true;
                    this.sent[key] = value;
                }
                if (changed == true) {
                    this.tran += 1;
                    this.send(JSON.stringify({ client: this.client, tran: this.tran.toString(), values: v }));
                }
            }

            this.timer = setTimeout(() => {
                this.timer = 0;
                this.sendData();
            }, this.reportInterval);
        }
    }

    /**
     * Розбирає отримані по сокету дані
     * @param msg те що прийшло по сокету
     */
    private receiveData(msg: MessageEvent): void {
        if (msg.data) {
            var parcel = JSON.parse(msg.data);
            if (this.tran < parcel.tran) {
                this.tran = parseInt(parcel.tran, 10);
                for (let i: number = 0; i < this.fields.length; i++) {
                    let key = this.fields[i];
                    let val = parcel.values[i];
                    if (this.sent[key] != val) {
                        this.sent[key] = val;
                        this.values[key] = val;
                        this.refreshInput(key, val);
                    }
                }
            } else {
                //відповідь на нашу посилку
                //Поновляємо всі readonly поля
                for (let i: number = 0; i < this.fields.length; i++) {
                    let key = this.fields[i];
                    let val = parcel.values[i];
                    for (let rIndex: number = 0; rIndex < this.readonlyFields.length; rIndex++) {
                        if (key == this.readonlyFields[rIndex]) {
                            this.sent[key] = val;
                            this.values[key] = val;
                            break;
                        }
                    }
                }
            }
            this.refreshOutput();
        }
    }

    /**
     * Включити/виключити повноекранний режим
     * */
    static toggleFullScreen(): void {
        let doc: any = window.document;
        let docEl: any = doc.documentElement;

        var requestFullScreen = docEl.requestFullscreen || docEl.mozRequestFullScreen || docEl.webkitRequestFullScreen || docEl.msRequestFullscreen;
        var cancelFullScreen = doc.exitFullscreen || doc.mozCancelFullScreen || doc.webkitExitFullscreen || doc.msExitFullscreen;

        if (!doc.fullscreenElement && !doc.mozFullScreenElement && !doc.webkitFullscreenElement && !doc.msFullscreenElement) {
            requestFullScreen.call(docEl);
        }
        else {
            cancelFullScreen.call(doc);
        }
    }

    /**
     * Проводить повторну ініціалізацію елементів у відповідності до нових розмірів екрану
     */
    UpdateLayout() {
        for (let i = 0; i < this.inputs.length; i++) {
            this.inputs[i].initLayout();
        }
        for (let o = 0; o < this.outputs.length; o++) {
            this.outputs[o].initLayout();
        }
    }

    /**
     * ініціалізує та реєструє всі елементи керування
     */
    private registerInputs() {
        var inputs = $(".input", this.form);

        inputs.each((index: number, val: any) => {
            let element: HTMLElement = val;
            var input: Input;
            if ($(element).hasClass("slider")) {
                input = new Slider(element);
            } else if ($(element).hasClass("btn")) {
                input = new Button(element);
            } else {
                input = new Input(element);
            }
            this.addInput(input);
        })
    }

    private addInput(input: Input): void {
        input.workSpace = this;
        this.inputs.push(input);
        input.saveValue();
    }

    /**
     * Ініціалізує та реєструє всі поля виводу інформації
     * */
    private registerOutputs() {
        var outputs = $(".output", this.form);

        outputs.each((index: number, val: any) => {
            let output: Output = null;
            output = new Output(val);
            if (!(this.values[output.name] != undefined)) {
                //Поле не зареєстроване, значить воно Readonly
                this.readonlyFields.push(output.name);
                this.values[output.name] = 0;
            }
            this.addOutput(output);
        })
    }

    private addOutput(output: Output): void {
        output.workSpace = this;
        this.outputs.push(output);
        output.loadValue();
    }

    /**
     * Розпочинає трансакцію вводу/виводу
     * */
    beginTransaction() {
        this.tranCount += 1;
    }


    /**
     * Закінчує трансакцію вводу/виводу
     * Під час закінчення трансакції - надсилається поточний стан
     * */
    endTransaction() {
        this.tranCount -= 1;
        if (this.tranCount === 0) {
            for (var i: number = 0; i < this.outputs.length; i++) {
                this.outputs[i].loadValue();
            }
        }
    }


    /**
     * Проставляє в елемент керування прийняте значення
     * @param key назва значення
     * @param value значення
     */
    private refreshInput(key: string, value: string): void {
        for (var i: number = 0; i < this.inputs.length; i++) {
            this.inputs[i].loadValue(key, value);
        }
    }

    /**
    * Проставляє в поля прийняті значення
    */
    private refreshOutput(): void {
        for (var i: number = 0; i < this.outputs.length; i++) {
            this.outputs[i].loadValue();
        }
    }
}


class Dictionary<T> {

    constructor(init?: { key: string; value: T; }[]) {
        if (init) {
            for (var x = 0; x < init.length; x++) {
                this[init[x].key] = init[x].value;
            }
        }
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

    saveValue(): void {
        if (!this.workSpace) return;
        this.workSpace.beginTransaction();
        let val: string = this.jElement.attr("value")
        if (val) {
            this.workSpace.values[this.name] = val;
        }
        this.workSpace.endTransaction();
    }

    loadValue(key: string, value: string): void {
        if (key == this.name) {
            this.jElement.attr("value", value);
        }
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
            this.saveValue();
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
        this.saveValue();
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
            this.saveValue();
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
        this.saveValue();
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

    saveValue(): void {
        if (!this.workSpace) return;
        this.workSpace.beginTransaction();
        let key_x = this.name + "_x";
        this.workSpace.values[key_x] = Slider.numToString(this.value.x);

        let key_y = this.name + "_y";
        this.workSpace.values[key_y] = Slider.numToString(this.value.y);
        this.workSpace.endTransaction();
    }

    loadValue(key: string, value: string): void {
        if (this.pressed == true) return;
        let key_x = this.name + "_x";
        let key_y = this.name + "_y";
        let refresh: boolean = false;
        if (key == key_x) {
            this.value.x = <any>value;
            refresh = true;
        }
        if (key == key_y) {
            this.value.y = <any>value;
            refresh = true;
        }
        if (refresh == true) {
            this.initLayout();
        }
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

class Button extends Input {

    pressed: boolean;
    sound_duration: number;
    audio: HTMLAudioElement = null;

    constructor(element: any) {
        super(element);

        let sound: string = this.jElement.data("sound");
        if (sound) {
            this.audio = new Audio(sound);
            this.audio.load();
        }
        this.sound_duration = this.jElement.data("sound-duration");

        if ("ontouchstart" in document.documentElement) {
            this.element.addEventListener('touchstart', (event: MouseEvent) => this.onTouchStart(event), false);
            this.element.addEventListener('touchend', (event: MouseEvent) => this.onTouchEnd(event), false);
        }
        else {
            this.element.addEventListener('mousedown', (event: MouseEvent) => this.onMouseDown(event), false);
            this.element.addEventListener('mouseup', (event: MouseEvent) => this.onMouseUp(event), false);
        }
    }


    private onTouchStart(event: MouseEvent): void {
        this.pressed = true;
        this.saveValue();
        this.Activate();
        this.playSound();
        event.preventDefault();
    }


    private onTouchEnd(event: MouseEvent): void {
        this.pressed = false;
        this.saveValue();
        event.preventDefault();
    }

    private onMouseDown(event: MouseEvent): void {
        this.pressed = true;
        this.saveValue();
        this.Activate();
        this.playSound();
        event.preventDefault();
    }

    private onMouseUp(event: MouseEvent): void {
        this.pressed = false;
        this.saveValue();
        event.preventDefault();
    }

    private Activate(): void {
        this.jElement.addClass("active");
        setTimeout(() => { this.jElement.removeClass("active") }, 200);
    }

    private playSound(): void {
        if (this.audio == null) return;
        if (!this.audio.paused) return;

        this.audio.currentTime = 0;
        let playPromise = this.audio.play();
        if (playPromise !== undefined) {
            playPromise.then(_ => {
                setTimeout(() => { this.audio.pause(); }, this.sound_duration);
            })
        }
    }

    saveValue(): void {
        if (!this.workSpace) return;
        this.workSpace.beginTransaction();
        let key = this.name;
        if (this.pressed) {
            this.workSpace.values[key] = "1";
        }
        else {
            this.workSpace.values[key] = "0";
        }
        this.workSpace.endTransaction();
    }
}

class Output {
    workSpace: WorkSpace;
    element: HTMLElement;
    jElement: JQuery;
    name: string;
    value: string;

    sound_duration: number;
    audio: HTMLAudioElement = null;

    constructor(element: any) {
        this.element = element;
        this.jElement = $(element);
        this.name = this.jElement.data("input");

        let sound: string = this.jElement.data("sound");
        if (sound) {
            this.audio = new Audio(sound);
            this.audio.load();

            this.sound_duration = this.jElement.data("sound-duration");

        }
    }

    loadValue(): void {
        if (!(this.workSpace.values[this.name] == undefined)) {
            let newValue = this.workSpace.values[this.name];
            if (this.element.tagName.toUpperCase() == "INPUT") {
                this.jElement.val(newValue);
            } if (this.element.tagName.toUpperCase() == "IMG") {
                if (newValue == "0") {
                    this.jElement.addClass("hidden")
                } else {
                    this.jElement.removeClass("hidden")
                }
            } if (this.element.classList.contains("progress-bar")) {
                this.jElement.width(<string>(newValue) + "%");
            }
            else {
                this.jElement.text(newValue);
            }

            if (this.value == "0" && !(newValue == "0")) {
                this.playSound();
            };
            this.value = newValue;
        }
    }

    initLayout(): void {

    }

    private playSound(): void {
        if (this.audio == null) return;
        if (!this.audio.paused) return;

        this.audio.currentTime = 0;
        let playPromise = this.audio.play();
        if (playPromise !== undefined && this.sound_duration !== undefined) {
            playPromise.then(_ => {
                setTimeout(() => { this.audio.pause(); }, this.sound_duration);
            })
        }
    }

}



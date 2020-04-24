class WorkSpace {
    public value: number = 0;

    private static form: HTMLFormElement;

    constructor() {
    }

    public static init(form: JQuery) {
        WorkSpace.form = <any>(form[0]);
        WorkSpace.form.oninput = () => {
            WorkSpace.form_input();
        }
        var controls = $(".slider", WorkSpace.form);
        controls.each((index: number, element: any) => {
            var slider: Slider = new Slider(element);
        })
    }

    private static form_input() {

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

}


class Point {
    x: number;
    y: number;
}


class Slider {

    private element: HTMLElement;
    private handle: HTMLElement;
    private pot: HTMLElement;

    private pressed: boolean = false;

    private moved: Point = new Point();

    private center: Point = new Point();


    public autoCenterX: boolean = false;
    public autoCenterY: boolean = false;


    constructor(element: any) {
        this.element = element;
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

        this.center.x = this.element.clientWidth / 2;
        this.center.y = this.element.clientHeight / 2;

        this.moved.x = this.center.x;
        this.moved.y = this.center.y;

        if ($(element).data("center")) {
            this.autoCenterX = true;
            this.autoCenterY = true;
        } else if ($(element).data("center-x")) {
            this.autoCenterX = true;
        } else if ($(element).data("center-y")) {
            this.autoCenterY = true;
        }

        this.drawInternal(true);

    }


    private onTouchStart(event): void {
        this.pressed = true;
    }
    private onTouchMove(event: TouchEvent) {
        // Prevent the browser from doing its default thing (scroll, zoom)
        event.preventDefault();
        if (this.pressed === true) {
            this.moved = Slider.pointFromTouch(this.element, event.targetTouches[0])
            this.drawInternal(false);
        }
    }
    private onTouchEnd(event): void {
        this.pressed = false;
        // If required reset position store variable
        if (this.autoCenterX)
            this.moved.x = this.center.x;
        if (this.autoCenterY)
            this.moved.y = this.center.y;
        this.drawInternal(true);
    }

    private onMouseDown(event): void {
        this.pressed = true;
    }
    private onMouseMove(event): void {
        if (this.pressed === true /*&& event.target === this.element*/) {
            this.moved = Slider.pointFromMouseEvent(this.element, event);
            this.drawInternal(false);
        }
    }
    private onMouseUp(event): void {
        this.pressed = false;
        // If required reset position store variable
        if (this.autoCenterX)
            this.moved.x = this.center.x;
        if (this.autoCenterY)
            this.moved.y = this.center.y;
        this.drawInternal(true);
    }


    private drawInternal(clip: boolean): void {
        if (clip) {
            if (this.moved.x < 0) this.moved.x = 0;
            if (this.moved.y < 0) this.moved.y = 0;
            if (this.moved.x > this.element.clientWidth) this.moved.x = this.element.clientWidth;
            if (this.moved.y > this.element.clientHeight) this.moved.y = this.element.clientHeight;
        }

        this.handle.style.left = '' + (this.moved.x - (this.handle.clientWidth / 2)) + 'px';
        this.handle.style.top = '' + (this.moved.y - (this.handle.clientHeight / 2)) + 'px';

        if (this.pot) {
            var pt: Point = new Point();
            pt.x = this.moved.x;
            pt.y = this.moved.y;
            if (pt.x < 0) pt.x = 0;
            if (pt.y < 0) pt.y = 0;
            if (pt.x > this.element.clientWidth) pt.x = this.element.clientWidth;
            if (pt.y > this.element.clientHeight) pt.y = this.element.clientHeight;

            this.pot.style.left = '' + (pt.x - (this.pot.clientWidth / 2)) + 'px';
            this.pot.style.top = '' + (pt.y - (this.pot.clientHeight / 2)) + 'px';
        }
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





class Slider {
    public value: number = 0;

    private form: HTMLFormElement;

    constructor(form: any) {
        var f: HTMLFormElement = form[0];
        this.form = f;
    }

    public init() {
        this.form.oninput = () => {
            this.form_input();
        }

    }

    private form_input() {

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







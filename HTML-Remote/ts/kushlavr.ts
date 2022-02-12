/**
 * 
 * */
class KVR {
    static load() {
        $.get("/api/setup")
            .done(function (data) {
                $.each(data, (name: string, value: string) => {
                    KVR.setValue(name, value);
                });
            })
            .fail(function () {
                console.log("error");
            });
    }

    static Autorefresh() {
        $.get("/api/values")
            .done(function (data) {
                $.each(data, function (name, value) {
                    KVR.setValue(name, value);
                });
                setTimeout(function () {
                    KVR.Autorefresh();
                }, 1000);
            })
            .fail(function () {
                console.log("error");
                setTimeout(function () {
                    KVR.Autorefresh();
                }, 1000);
            });
    }

    static setValue(inputName, value) {
        console.log("Name: " + inputName + ", Value: " + value);
        let form = $("#form");
        let input = $("[name=" + inputName + "]", form);
        if (input.length > 0) {
            if (input[0].tagName === "INPUT") {
                input.val(value);
            } else if (input[0].tagName === "SELECT") {
                input.val(value);
            } else {
                input.html(value);
            }
        }
    }
}

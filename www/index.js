//-- Init ---------------------------------------------------------------------

function init() {
    //-- AJAX errors notification --
    $(document).ajaxError(() => {
        message("I did not expected this, but there was an error. Sorry for this.");
    });
    $(document).ajaxStart(showProgress);
    $(document).ajaxStop(hideProgress);

    //-- Register save --
    $("#save").click(() => {
        $.ajax({
            type: "POST",
            url: "/settings.json",
            data: $("#settings").serialize(), // serializes the form's elements.
            success: function () {
                message("Settings saved");
                getSettings();
            }
        });
    });

    //-- Register dangerous callbacks --
    $("#reboot").click(() => {
        $.ajax({
            type: "GET",
            url: "/reboot",
            success: function () {
                message("System has rebooted", 0);
                $("button").attr("disabled", "disabled");
            }
        });
    });

    //-- Hide notification --
    $("#message").click(() => {
        $("#message").slideUp(100);
    })
}


//-- Progress bar -------------------------------------------------------------

function showProgress() {
    $("#progress").show();
}

function hideProgress() {
    $("#progress").hide();
}

function message(text, timeout_s = 5) {
    var $msg = $("#message");
    if (text) {
        $msg.html(text).slideDown(200);
        if (timeout_s) {
            setTimeout(() => {
                $msg.slideUp(200);
            }, timeout_s * 1000);
        }
    } else {
        $msg.hide();
    }
}

//-- Form ---------------------------------------------------------------------

//-- AJAX ---------------------------------------------------------------------

/** Retrieve information from the chip (id, wireless access points, ...) */
function getSettings() {
    showProgress();
    $.getJSON("/settings.json").done((infos) => {
        //-- Chip ID ----------------------------------------------------------
        $("#chip_id").text(infos.chip_id);
        $("input[name='board_uid']").attr("placeholder", infos.chip_id);

        //-- Fill form --------------------------------------------------------
        for (let k in infos.settings) {
            $(`input[name="${k}"]`).val(infos.settings[k]);
        }
    }).fail(() => {
        message("Failed to load chip info");
    }).always(hideProgress);
}

//-- Main ---------------------------------------------------------------------

$(() => {
    init();
    getSettings();
});
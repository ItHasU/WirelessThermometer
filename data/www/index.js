//-- Init ---------------------------------------------------------------------

function init() {
    //-- Register save --
    $("#save").click(() => {
        $.ajax({
            type: "POST",
            url: "/settings.json",
            data: $("#settings").serialize(), // serializes the form's elements.
            success: function () {
                alert("Done"); // show response from the php script.
            }
        });
    });

    //-- Register reboot --

}


//-- Progress bar -------------------------------------------------------------

function showProgress() {
    $("#progress").show();
}

function hideProgress() {
    $("#progress").hide();
}

function errorMessage(text) {
    if (text) {
        $("#error").html(text).show();
    } else {
        $("#error").hide();
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

        //-- Fill form --------------------------------------------------------
        for (let k in infos.settings) {
            $(`input[name="${k}"]`).val(infos.settings[k]);
        }
    }).fail(() => {
        errorMessage("Failed to load chip info");
    }).always(hideProgress);
}

//-- Main ---------------------------------------------------------------------

$(() => {
    init();
    getSettings();
});
(function ($) {
  Drupal.behaviors.progressiveProfiling = {
    attach: function (context, settings) {
      if ($('form#progressive-profiling-form', context).get(0)) {
        var profiling_form = $('form#progressive-profiling-form', context);
        Drupal.CTools.Modal.show('modal-popup-small');
        $('#modal-content').addClass('modal-content-profiling').empty().prepend(profiling_form);
        $('#modal-title').html(profiling_form.find('#edit-title').text());
        profiling_form.find('#edit-title').remove();
      }
    }
  }
}(jQuery));

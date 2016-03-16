(function ($) {
  Drupal.behaviors.progressiveProfiling = {
    attach: function (context, settings) {
      if ($('form#progressive-profiling-form').get(0), context) {
        var profiling_form = $('form#progressive-profiling-form');
        Drupal.CTools.Modal.show('modal-popup-small');
        $('#modal-content').empty().prepend(profiling_form);
        $('#modal-title').html(profiling_form.find('#edit-title').text());
        profiling_form.find('#edit-title').remove();
      }
    }
  }
}(jQuery));

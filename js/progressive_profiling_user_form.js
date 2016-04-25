(function ($) {
  Drupal.behaviors.progressiveProfilingUserForm = {
    attach: function (context, settings) {
      if (settings.progressive_profiling_fields !== 'undefined') {
        for (var index = 0; index < settings.progressive_profiling_fields.length; index++) {
          $('form#user-profile-form', context).find('[name^="' + settings.progressive_profiling_fields[index] + '["]').bind('change', function() {
            var field_name = $(this).attr('name');
            var words = field_name.split('[');
            var changed_profile_fields_val = $('form#user-profile-form input[name="changed_profile_fields"]').val();
            $('form#user-profile-form input[name="changed_profile_fields"]').val(changed_profile_fields_val + words[0]+',');
          });
        }
      }
    }
  }
}(jQuery));

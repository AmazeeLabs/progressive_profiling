<?php

/**
 * @file
 *  Administration pages for the module.
 */

/**
 * Menu callback for the general progressive profiling settings page.
 */
function progressive_profiling_settings_form() {
  // We intentionally left this string untranslated because it is supposed to be
  // used only in development, as a placeholder.
  $conditions = variable_get('progressive_profiling_conditions', array());
  ctools_include('context');
  $conditions_configuration = array();
  $base_admin_url = progressive_profiling_base_admin_url();
  if (!empty($conditions)) {
    $contexts = progressive_profiling_available_contexts();
      foreach ($conditions as $condition_key => $plugin_conf) {
        $plugin = ctools_get_access_plugin($plugin_conf['name']);
        $conditions_configuration[] = ctools_access_summary($plugin, $contexts, $plugin_conf) . ' ' . l(t('edit condition'), $base_admin_url . '/settings/condition/edit/' . $condition_key, array('query' => drupal_get_destination()));
      }
      $conditions_logic = variable_get('progressive_profiling_conditions_logic', 'and');
      $conditions_configuration[] = t('<strong>Logic</strong>: @logic', array('@logic' => ($conditions_logic === 'or') ? t('Only one criteria must pass.') : t('All criteria must pass.'))) . ' ' . l(t('edit logic'), $base_admin_url . '/settings/condition/edit_logic', array('query' => drupal_get_destination()));
  }
  else {
    $conditions_configuration[] ='<div>' . t('No conditions defined yet.') . '</div>';
  }
  $conditions_configuration[] = implode(' | ', array(
    l(t('add condition'), $base_admin_url . '/settings/condition/add', array('query' => drupal_get_destination()))
  ));

  $form['profile_conditions'] = array(
    '#type' => 'fieldset',
    '#title' => t('Conditions'),
    '#description' => t('Define the global conditions of when the users should be asked to input additional data to their profile.')
  );
  $form['profile_conditions']['conditions'] = array(
    '#type' => 'item',
    '#markup' => implode('<br />', $conditions_configuration),
  );

  $form['progressive_profiling_time_interval'] = array(
    '#type' => 'select',
    '#options' => array(0 => t('No interval (not recommended, use it for testing)')) + drupal_map_assoc(array(60, 300, 600, 1800, 3600, 10800, 21600, 32400, 43200, 86400, 172800, 259200, 604800), 'format_interval'),
    '#title' => t('Time interval'),
    '#description' => t('Please select the time interval between the last time when the user filled in the fields for the profile and when the user is asked again to do that. Be aware that shorter intervals could make the users angry.'),
    '#default_value' => variable_get('progressive_profiling_time_interval', 0),
  );

  return system_settings_form($form);
}

/**
 * Page callback for adding a new condition for the general settings.
 *
 * The following functions are a bit duplicated code with the ones above, but
 * they do not need the field_name variable.
 */
function progressive_profiling_settings_conditions_add() {
  ctools_include('context');
  return drupal_get_form('progressive_profiling_settings_condition_form');
}

/**
 * Page callback for editing a condition for the general settings.
 *
 * @param string $condition_key
 *  The condition identifier.
 */
function progressive_profiling_settings_conditions_edit($condition_key) {
  ctools_include('context');
  $form_state = array();
  // We directly want to show the condition configuration, so we prefill the
  // values from the first step (from progressive_profiling_condition_form())
  // into the storage of the form_state.
  $form_state['storage']['condition_configuration'] = TRUE;
  $form_state['storage']['condition_values']['condition_key'] = $condition_key;
  $conditions = variable_get('progressive_profiling_conditions', array());
  $form_state['storage']['condition_values']['condition_type'] = $conditions[$condition_key]['name'];

  return drupal_build_form('progressive_profiling_settings_condition_form', $form_state);
}

/**
 * Form constructor for adding/editing a general condition.
 */
function progressive_profiling_settings_condition_form($form, &$form_state) {
  // The calling function must include the ctools contexts.
  // @see progressive_profiling_settings_conditions_add().

  // Check if we should show the condition configuration form.
  if (isset($form_state['storage']['condition_configuration'])) {
    return progressive_profiling_settings_condition_configuration_form($form, $form_state);
  }

  // We can have some contexts, as the logged in user, the term (if we are on
  // a taxonomy term page) or a node (for the full node view pages, see
  // progressive_profiling_available_contexts()).
  // @todo: the context options can be extended with a form alter, maybe would
  // be nicer with a dedicated hook.
  $contexts = progressive_profiling_available_contexts();
  $plugins = ctools_get_relevant_access_plugins($contexts);

  $options = array();
  foreach ($plugins as $id => $plugin) {
    $options[$id] = $plugin['title'];
  }
  asort($options);
  $form['condition_type'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#title' => t('Condition type')
  );
  $form['actions']['#type'] = 'actions';
  $form['actions']['next'] = array(
    '#type' => 'submit',
    '#value' => t('Next'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => progressive_profiling_base_admin_url() . '/settings',
  );
  $form_state['step'] = 'condition';

  return $form;
}

/**
 * Form constructor for the condition configuration form.
 *
 * This is the second step of adding a condition (see
 * progressive_profiling_settings_condition_form()).
 */
function progressive_profiling_settings_condition_configuration_form($form, &$form_state) {
  $plugin_name = $form_state['storage']['condition_values']['condition_type'];
  $condition_key = !empty($form_state['storage']['condition_values']['condition_key']) ? $form_state['storage']['condition_values']['condition_key'] : NULL;
  $plugin = ctools_get_access_plugin($plugin_name);
  $form['settings'] = array('#tree' => TRUE);
  $form_state['plugin'] = $plugin;

  if (empty($condition_key)) {
    $conditions = array();
  }
  else {
    $conditions = variable_get('progressive_profiling_conditions', array());
    $conditions = !empty($conditions[$condition_key]) ? $conditions[$condition_key] : array();
  }

  $contexts = progressive_profiling_available_contexts();
  if (isset($plugin['required context'])) {
    if (empty($conditions['context'])) {
      $conditions['context'] = '';
    }
    $form['context'] = ctools_context_selector($contexts, $plugin['required context'], $conditions['context']);
  }

  if ($function = ctools_plugin_get_function($plugin, 'settings form')) {
    $settings = !empty($conditions['settings']) ? $conditions['settings'] : $plugin['default'];
    $form = $function($form, $form_state, $settings);
  }
  if (!empty($condition_key)) {
    $form['condition_key'] = array(
      '#type' => 'value',
      '#value' => $condition_key,
    );
  }
  $form['not'] = array(
    '#type' => 'checkbox',
    '#title' => t('Reverse (NOT)'),
    '#default_value' => !empty($conditions['not']),
  );

  $form['actions']['#type'] = 'actions';
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  // @todo: we should maybe at least add a js confirmation popup here.
  $form['actions']['remove'] = array(
    '#type' => 'submit',
    '#value' => t('Remove'),
    '#remove' => TRUE,
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => progressive_profiling_base_admin_url() . '/settings',
  );
  $form_state['step'] = 'condition_configuration';
  return $form;
}

/**
 * Validation handler for the general settings condition configuration form.
 *
 * Basically, it just forwards the validation to the callback defined by the
 * ctools access plugin.
 */
function progressive_profiling_settings_condition_form_validate($form, &$form_state) {
  if (!empty($form_state['clicked_button']['#remove'])) {
    return;
  }

  if ($form_state['step'] == 'condition_configuration') {
    try {
      // Do some checks before, to be sure we have valid data.
      if (empty($form_state['storage']['condition_values'])) {
        throw new Exception(t('There are no condition available for editing.'));
      }
      if (empty($form_state['storage']['condition_values']['condition_type'])) {
        throw new Exception(t('No available condition type to be set.'));
      }
      if (!empty($form_state['plugin']) && $function = ctools_plugin_get_function($form_state['plugin'], 'settings form validate')) {
        $function($form, $form_state);
      }
    }
    catch (Exception $e) {
      form_set_error('', $e->getMessage());
      watchdog_exception('progressive_profiling', $e);
    }
  }
}

/**
 * Submit handler for the condition configuration form.
 */
function progressive_profiling_settings_condition_form_submit($form, &$form_state) {
  if ($form_state['step'] == 'condition') {
    $form_state['storage']['condition_values'] = $form_state['values'];
    $form_state['storage']['condition_configuration'] = TRUE;
    $form_state['rebuild'] = TRUE;
  }
  elseif ($form_state['step'] == 'condition_configuration') {
    if ($function = ctools_plugin_get_function($form_state['plugin'], 'settings form submit')) {
      $function($form, $form_state);
    }
    // Just update or remove the configuration.
    $conditions = variable_get('progressive_profiling_conditions', array());
    $condition_key = !empty($form_state['values']['condition_key']) ? $form_state['values']['condition_key'] : time();
    if (!empty($form_state['clicked_button']['#remove'])) {
      unset($conditions[$condition_key]);
    }
    else {
      $conditions[$condition_key]['settings'] = $form_state['values']['settings'];
      $conditions[$condition_key]['name'] = $form_state['storage']['condition_values']['condition_type'];
      $conditions[$condition_key]['not'] = $form_state['values']['not'];
      if (!empty($form_state['values']['context'])) {
        $conditions[$condition_key]['context'] = $form_state['values']['context'];
      }
    }
    variable_set('progressive_profiling_conditions', $conditions);
  }
}

/**
 * Form constructor for the edit general conditions logic form.
 */
function progressive_profiling_settings_conditions_edit_logic_form($form, &$form_state) {
  // Setting for the logic of the conditions (if all should pass, or only one is
  // needed to pass).
  $logic = variable_get('progressive_profiling_conditions_logic', 'and');
  $form['progressive_profiling_conditions_logic'] = array(
    '#type' => 'radios',
    '#options' => array(
      'and' => t('All criteria must pass.'),
      'or' => t('Only one criteria must pass.'),
    ),
    '#default_value' => $logic,
  );
  $form = system_settings_form($form);
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => progressive_profiling_base_admin_url() . '/settings',
  );
  return $form;
}

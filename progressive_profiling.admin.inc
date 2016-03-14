<?php

/**
 * @file
 *  Administration pages for the module.
 */

/**
 * Menu callback for the questions administration page.
 */
function progressive_profiling_fields() {
  return array(
    'questions' => drupal_get_form('progressive_profiling_fields_table_form'),
    'add_form' => drupal_get_form('progressive_profiling_add_field_form'),
  );
}

/**
 * Form constructor for the fields list table.
 */
function progressive_profiling_fields_table_form($form, &$form_state) {
  ctools_include('context');
  $fields = progressive_profiling_field_load_all();
  $field_info = field_info_instances('user', 'user');
  $header = array(
    'field_name' => t('Field name'),
    'conditions' => t('Conditions'),
    'status' => t('Status'),
    'actions' => t('Actions'),
    'weight' => t('Weight'));
  $rows = array();
  $field_names = array();
  $weight_delta = round(count($fields) / 2) + 10;
  $base_admin_url = progressive_profiling_base_admin_url();
  if (!empty($fields)) {
    foreach ($fields as $field_name => $field) {
      $row = array();
      // Field name column.
      // If the field was deleted from the user, we still want to show it, so that
      // the admin can delete if from the progressive profiling.
      // @todo: should we maybe delete the field automatically?
      $row['field_name'] = !empty($field_info[$field_name]) ? $field_info[$field_name]['label'] . ' (' . $field_name . ')' : t('--Missing field--') . ' (' . $field_name . ')';

      // Conditions column.
      $condition_configuration = array();
      if (!empty($field->configuration['conditions'])) {
        $contexts = progressive_profiling_available_contexts();
        foreach ($field->configuration['conditions'] as $plugin_name => $plugin_conf) {
          $plugin = ctools_get_access_plugin($plugin_name);
          $condition_configuration[] = ctools_access_summary($plugin, $contexts, $plugin_conf) . ' ' . l(t('edit condition'), $base_admin_url . '/fields/conditions/' . $field_name . '/edit/' . $plugin_name, array('query' => drupal_get_destination()));
        }
        $conditions_logic = !empty($field->configuration['conditions_logic']) ? $field->configuration['conditions_logic'] : 'and';
        $condition_configuration[] = t('<strong>Logic</strong>: @logic', array('@logic' => ($conditions_logic === 'or') ? t('Only one criteria must pass.') : t('All criteria must pass.'))) . ' ' . l(t('edit logic'), $base_admin_url . '/fields/conditions/' . $field_name . '/edit_set', array('query' => drupal_get_destination()));
      }
      else {
        $condition_configuration[] = t('No conditions defined, the field will be asked to all the users.');
      }
      // In case we will have some more links, we can just add them to this array.
      // (This is expected maybe for the future.)
      $condition_configuration[] = implode(' | ', array(
        l(t('add condition'), $base_admin_url . '/fields/conditions/' . $field_name . '/add', array('query' => drupal_get_destination())),
      ));
      $row['conf'] = implode('<br />', $condition_configuration);

      // Status column.
      $actions = array();
      if (!empty($field->status)) {
        $row['status'] = '<span style="color: green">' . t('Enabled') . '</span>';
        $actions['status'] = l(t('disable'), $base_admin_url . '/fields/disable/' . $field_name, array('query' => drupal_get_destination()));
      }
      else {
        $row['status'] = '<span style="color: red">' . t('Disabled') . '</span>';
        $actions['status'] = l(t('enable'), $base_admin_url . '/fields/enable/' . $field_name, array('query' => drupal_get_destination()));
      }

      // Actions column.
      // Currently, the edit operation is not needed. We have the code implemented
      // (the menu item and the callback), so when we will need it we can just
      // activate them again.
      //$actions['edit'] = l(t('edit'), $base_admin_url . '/fields/edit/' . $field_name, array('query' => drupal_get_destination()));
      $actions['delete'] = l(t('delete'), $base_admin_url . '/fields/delete/' . $field_name, array('query' => drupal_get_destination()));
      $row['actions'] = implode(' | ', $actions);

      $rows[$field_name] = array('data' => $row, 'class' => array('draggable'));
      $field_names[] = $field_name;
      $form['weight_' . $field_name] = array(
        '#type' => 'weight',
        '#default_value' => $field->weight,
        '#delta' => $weight_delta,
        '#attributes' => array('class' => array('component-field-weight')),
        '#title' => t('Weight'),
        '#title_display' => 'invisible',
      );
    }
    $form['field_names'] = array(
      '#type' => 'value',
      '#value' => $field_names,
    );
    $form['#header'] = $header;
    $form['#rows'] = $rows;
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
    );
  }
  else {
    $form['no_fields'] = array(
      '#markup' => '<h3>' . t('There are no fields configured yet, please add them using the form below.') . '</h3>',
    );
  }
  return $form;
}

/**
 * Submit handler for the fields list table. It updates the weights of the
 * fields.
 */
function progressive_profiling_fields_table_form_submit($form, &$form_state) {
  foreach ($form_state['values']['field_names'] as $field_name) {
    $field = progressive_profiling_field_load($field_name);
    $field->weight = $form_state['values']['weight_' . $field_name];
    progressive_profiling_field_save($field);
  }
}

/**
 * Theme for the site_phase_admin_form form.
 */
function theme_progressive_profiling_fields_table_form(&$variables) {
  $form = $variables['form'];
  $table_id = 'progressive-profiling-table';
  $output = '';
  if (!empty($form['#rows'])) {
    foreach ($form['#rows'] as $field_name => &$value) {
      $form['#rows'][$field_name]['data']['weight'] = drupal_render($form['weight_' . $field_name]);
    }
    $output = theme('table', array(
      'header' => $form['#header'],
      'rows' => $form['#rows'],
      'attributes' => array('id' => $table_id)
    ));
    drupal_add_tabledrag($table_id, 'order', 'sibling', 'component-field-weight');
  }
  $output .= drupal_render_children($form);
  return $output;
}

/**
 * Form constructor for the add field form.
 *
 * The add field form is just a simple select field where the user can choose
 * to flag a field to be used in the progressive profiling.
 */
function progressive_profiling_add_field_form() {
  $available_fields = progressive_profiling_get_available_fields();
  $options = array();
  foreach ($available_fields as $field_name => $field) {
    $options[$field_name] = $field['label'] . ' (' . $field_name . ')';
  }
  $form['add'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add field'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['add']['field_name'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#title' => t('Field name'),
    '#description' => t('Please select the field which you want to add to the progressive profiling fields pool.'),
  );
  $form['add']['actions'] = array(
    '#type' => 'actions',
  );
  $form['add']['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
  );
  $form['add']['actions']['create'] = array(
    '#markup' => '<span>' . t('or') . '</span> ' . l(t('Create a new field'), 'admin/config/people/accounts/fields', array('query' => drupal_get_destination())),
  );
  return $form;
}

/**
 * Submit handler for the add field form.
 */
function progressive_profiling_add_field_form_submit(&$form, &$form_state) {
  $form_state['redirect'] = array(progressive_profiling_base_admin_url() . '/fields/add/' . $form_state['values']['field_name'], array('query' => drupal_get_destination()));
}

/**
 * Form constructor for adding/editing a field for the progressive profile.
 *
 * @param mixt $field
 *  The progressive profile field configuration, or the field_name, in case
 *  there is not already a configuration. When the $field is an object, this
 *  means that we are actually on the edit form.
 */
function progressive_profiling_fields_add_edit_field_form($form, &$form_state, $field) {
  $field_name = is_string($field) ? $field : $field->field_name;
  $field_info = field_info_instance('user', $field_name, 'user');
  if (empty($field_info)) {
    drupal_set_message(t('The field %field_name is not a valid field.', array('%field_name' => $field_name)), 'error');
    drupal_goto();
  }
  $form['field_name'] = array(
    '#type' => 'value',
    '#value' => $field_name,
  );
  $lead = is_string($field) ? t('Add field %field_name?', array('%field_name' => $field_info['label'])) : t('Edit field %field_name', array('%field_name' => $field_info['label']));
  $form['lead'] = array(
    '#markup' => '<h3>' . $lead . '</h3>',
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => progressive_profiling_base_admin_url(),
  );
  return $form;
}

/**
 * Submit handler for the add/edit field form.
 */
function progressive_profiling_fields_add_edit_field_form_submit(&$form, &$form_state) {
  $field_configuration = progressive_profiling_field_load($form_state['values']['field_name']);
  if (empty($field_configuration)) {
    $field_configuration = new stdClass();
    $field_configuration->field_name = $form_state['values']['field_name'];
    progressive_profiling_field_object_prepare($field_configuration);
  }
  progressive_profiling_field_save($field_configuration);
}

/**
 * Confirmation form for disabling a field.
 *
 * @param object $field
 *  A field object returned by progressive_profiling_field_load().
 */
function progressive_profiling_fields_disable_confirm($form, &$form_state, $field) {
  $field_info = field_info_instance('user', $field->field_name, 'user');
  $form['field_name'] = array(
    '#type' => 'value',
    '#value' => $field->field_name,
  );
  return confirm_form($form, t('Are you sure you want to disable the field?'), progressive_profiling_base_admin_url(), t('Are you sure you want to disable the field %field from the progressive profiling?', array('%field' => $field_info['label'])));
}

/**
 * Submit handler for the field disable confirm form.
 */
function progressive_profiling_fields_disable_confirm_submit($form, &$form_state) {
  $field = progressive_profiling_field_load($form_state['values']['field_name']);
  $field->status = 0;
  progressive_profiling_field_save($field);
  drupal_set_message(t('The field has been disabled.'));
}

/**
 * Confirmation form for enabling a field.
 *
 * @param object $field
 *  A field object returned by progressive_profiling_field_load().
 */
function progressive_profiling_fields_enable_confirm($form, &$form_state, $field) {
  $field_info = field_info_instance('user', $field->field_name, 'user');
  $form['field_name'] = array(
    '#type' => 'value',
    '#value' => $field->field_name,
  );
  return confirm_form($form, t('Are you sure you want to enable the field?'), progressive_profiling_base_admin_url(), t('Are you sure you want to enable the field %field for the progressive profiling?', array('%field' => $field_info['label'])));
}

/**
 * Submit handler for the field enable confirm form.
 */
function progressive_profiling_fields_enable_confirm_submit($form, &$form_state) {
  $field = progressive_profiling_field_load($form_state['values']['field_name']);
  $field->status = 1;
  progressive_profiling_field_save($field);
  drupal_set_message(t('The field has been enabled.'));
}

/**
 * Confirmation form for enabling a field.
 *
 * @param object $field
 *  A field object returned by progressive_profiling_field_load().
 */
function progressive_profiling_fields_delete_confirm($form, &$form_state, $field) {
  $field_info = field_info_instance('user', $field->field_name, 'user');
  $form['field_name'] = array(
    '#type' => 'value',
    '#value' => $field->field_name,
  );
  return confirm_form($form, t('Are you sure you want to remove the field?'), progressive_profiling_base_admin_url(), t('Are you sure you want to remove the field %field from the progressive profiling?', array('%field' => $field_info['label'])));
}

/**
 * Submit handler for the field enable confirm form.
 */
function progressive_profiling_fields_delete_confirm_submit($form, &$form_state) {
  progressive_profiling_field_delete($form_state['values']['field_name']);
  drupal_set_message(t('The field has been removed.'));
}

/**
 * Page callback for adding a the condition to the field.
 *
 * @param object $field
 *  The profiling field object.
 */
function progressive_profiling_fields_conditions_add($field) {
  ctools_include('context');
  return drupal_get_form('progressive_profiling_condition_form', $field);
}

/**
 * Page callback for editing a condition.
 */
function progressive_profiling_fields_conditions_edit($field, $plugin_name) {
  ctools_include('context');
  $form_state = array();
  $form_state['build_info']['args'] = array($field);
  // We directly want to show the condition configuration, so we prefill the
  // values from the first step (from progressive_profiling_condition_form())
  // into the storage of the form_state.
  $form_state['storage']['condition_configuration'] = TRUE;
  $form_state['storage']['condition_values']['condition_type'] = $plugin_name;
  $form_state['storage']['condition_values']['field_name'] = $field->field_name;

  return drupal_build_form('progressive_profiling_condition_form', $form_state);
}

/**
 * Form constructor for a field condition.
 *
 * @param object $field
 *  The profiling field object.
 */
function progressive_profiling_condition_form($form, &$form_state, $field) {
  // The calling function must include the ctools contexts.
  // @see progressive_profiling_fields_conditions_add().

  // Check if we should show the condition configuration form.
  if (isset($form_state['storage']['condition_configuration'])) {
    return progressive_profiling_condition_configuration_form($form, $form_state, $field);
  }
  $plugins = ctools_get_relevant_access_plugins(array());
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
  $form['field_name'] = array(
    '#type' => 'value',
    '#value' => $field->field_name,
  );
  $form['actions']['#type'] = 'actions';
  $form['actions']['next'] = array(
    '#type' => 'submit',
    '#value' => t('Next'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => progressive_profiling_base_admin_url(),
  );
  $form_state['step'] = 'condition';

  return $form;
}

/**
 * Form constructor for the condition configuration (the second step of the
 * add/edit condition).
 *
 * @param object $field
 *  The profiling field object.
 */
function progressive_profiling_condition_configuration_form($form, &$form_state, $field) {
  $plugin_name = $form_state['storage']['condition_values']['condition_type'];
  $plugin = ctools_get_access_plugin($plugin_name);
  $form['settings'] = array('#tree' => TRUE);
  $form_state['plugin'] = $plugin;

  $condition_configuration = !empty($field->configuration['conditions'][$plugin_name]) ? $field->configuration['conditions'][$plugin_name] : array();

  $contexts['logged-in-user'] = ctools_access_get_loggedin_context();
  if (isset($plugin['required context'])) {
    if (empty($condition_configuration['context'])) {
      $condition_configuration['context'] = 'logged-in-user';
    }
    $form['context'] = ctools_context_selector($contexts, $plugin['required context'], $condition_configuration['context']);
  }

  if ($function = ctools_plugin_get_function($plugin, 'settings form')) {
    $settings = !empty($condition_configuration['settings']) ? $condition_configuration['settings'] : $plugin['default'];
    $form = $function($form, $form_state, $settings);
  }
  $form['not'] = array(
    '#type' => 'checkbox',
    '#title' => t('Reverse (NOT)'),
    '#default_value' => !empty($condition_configuration['not']),
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
    '#href' => progressive_profiling_base_admin_url(),
  );
  $form_state['step'] = 'condition_configuration';
  return $form;
}

/**
 * Validation handler for the condition configuration form.
 *
 * Basically, it just forwards the validation to the callback defined by the
 * ctools access plugin.
 */
function progressive_profiling_condition_form_validate($form, &$form_state) {
  if (!empty($form_state['clicked_button']['#remove'])) {
    return;
  }

  if ($form_state['step'] == 'condition_configuration') {
    try {
      // Do some checks before, to be sure we have valid data.
      if (empty($form_state['storage']['condition_values'])) {
        throw new Exception(t('There are no condition available for editing.'));
      }
      if (empty($form_state['storage']['condition_values']['field_name'])) {
        throw new Exception(t('No available field name for which to configure the condition.'));
      }
      if (empty($form_state['storage']['condition_values']['condition_type'])) {
        throw new Exception(t('No available condition type to be set.'));
      }
      $field = progressive_profiling_field_load($form_state['storage']['condition_values']['field_name']);
      if (empty($field)) {
        throw new Exception(t('There is no profiling field with the name: @field_name.', array('@field_name' => $form_state['storage']['condition_values']['field_name'])));
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
function progressive_profiling_condition_form_submit($form, &$form_state) {
  if ($form_state['step'] == 'condition') {
    $form_state['storage']['condition_values'] = $form_state['values'];
    $form_state['storage']['condition_configuration'] = TRUE;
    $form_state['rebuild'] = TRUE;
  }
  elseif ($form_state['step'] == 'condition_configuration') {
    if ($function = ctools_plugin_get_function($form_state['plugin'], 'settings form submit')) {
      $function($form, $form_state);
    }
    // Just update the configuration for this condition (or remove it).
    $field = progressive_profiling_field_load($form_state['storage']['condition_values']['field_name']);
    if (!empty($form_state['clicked_button']['#remove'])) {
      unset($field->configuration['conditions'][$form_state['storage']['condition_values']['condition_type']]);
      if (empty($field->configuration['conditions'])) {
        unset($field->configuration['conditions']);
      }
    }
    else {
      $field->configuration['conditions'][$form_state['storage']['condition_values']['condition_type']]['settings'] = $form_state['values']['settings'];
      $field->configuration['conditions'][$form_state['storage']['condition_values']['condition_type']]['not'] = $form_state['values']['not'];
      if (!empty($form_state['values']['context'])) {
        $field->configuration['conditions'][$form_state['storage']['condition_values']['condition_type']]['context'] = $form_state['values']['context'];
      }
    }
    progressive_profiling_field_save($field);
  }
}

/**
 * Form constructor for the conditions settings form.
 */
function progressive_profiling_fields_conditions_settings_form($form, &$form_state, $field) {
  // Setting for the logic of the conditions (if all should pass, or only one is
  // needed to pass).
  $form['logic'] = array(
    '#type' => 'radios',
    '#options' => array(
      'and' => t('All criteria must pass.'),
      'or' => t('Only one criteria must pass.'),
    ),
    '#default_value' => !empty($field->configuration['conditions_logic']) ? $field->configuration['conditions_logic'] : 'and',
  );
  $form['field_name'] = array(
    '#type' => 'value',
    '#value' => $field->field_name,
  );
  $form['actions']['#type'] = 'actions';
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => progressive_profiling_base_admin_url(),
  );
  return $form;
}

/**
 * Submit handler for the conditions settings form.
 */
function progressive_profiling_fields_conditions_settings_form_submit(&$form, &$form_state) {
  $field = progressive_profiling_field_load($form_state['values']['field_name']);
  $field->configuration['conditions_logic'] = $form_state['values']['logic'];
  progressive_profiling_field_save($field);
  drupal_set_message(t('The conditions settings have been saved.'));
}

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
      foreach ($conditions as $plugin_name => $plugin_conf) {
        $plugin = ctools_get_access_plugin($plugin_name);
        $conditions_configuration[] = ctools_access_summary($plugin, $contexts, $plugin_conf) . ' ' . l(t('edit condition'), $base_admin_url . '/settings/condition/edit/' . $plugin_name, array('query' => drupal_get_destination()));
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

  return $form;
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
 * @param string $plugin_name
 *  The condition plugin name.
 */
function progressive_profiling_settings_conditions_edit($plugin_name) {
  ctools_include('context');
  $form_state = array();
  // We directly want to show the condition configuration, so we prefill the
  // values from the first step (from progressive_profiling_condition_form())
  // into the storage of the form_state.
  $form_state['storage']['condition_configuration'] = TRUE;
  $form_state['storage']['condition_values']['condition_type'] = $plugin_name;

  return drupal_build_form('progressive_profiling_settings_condition_form', $form_state);
}

/**
 * Returns an array with all the contexts currently available for conditions.
 */
function progressive_profiling_available_contexts() {
  $contexts['logged-in-user'] = ctools_access_get_loggedin_context();
  $contexts['node'] = ctools_context_create_empty('entity:node');
  $contexts['node']->identifier = t('Node being viewed');
  $contexts['term'] = ctools_context_create_empty('entity:taxonomy_term');
  $contexts['term']->identifier = t('Term being viewed');
  return $contexts;
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
  $plugin = ctools_get_access_plugin($plugin_name);
  $form['settings'] = array('#tree' => TRUE);
  $form_state['plugin'] = $plugin;

  $conditions = variable_get('progressive_profiling_conditions', array());
  $conditions = !empty($conditions[$plugin_name]) ? $conditions[$plugin_name] : array();

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
    if (!empty($form_state['clicked_button']['#remove'])) {
      unset($conditions[$form_state['storage']['condition_values']['condition_type']]);
    }
    else {
      $conditions[$form_state['storage']['condition_values']['condition_type']]['settings'] = $form_state['values']['settings'];
      $conditions[$form_state['storage']['condition_values']['condition_type']]['not'] = $form_state['values']['not'];
      if (!empty($form_state['values']['context'])) {
        $conditions[$form_state['storage']['condition_values']['condition_type']]['context'] = $form_state['values']['context'];
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

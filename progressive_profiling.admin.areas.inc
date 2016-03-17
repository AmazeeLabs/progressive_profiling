<?php

/**
 * @file
 *  Contains the page callbacks for the areas administration.
 */

/**
 * Menu callback for the questions administration page.
 */
function progressive_profiling_areas() {
  $areas = progressive_profiling_areas_table();
  return array(
    'areas' => !empty($areas) ? $areas : array('#markup' => '<h3>' . t('No areas defined yet.') . '</h3>'),
    'add_form' => drupal_get_form('progressive_profiling_areas_add_edit_form'),
  );
}

/**
 * The list of all the areas structured as a table.
 */
function progressive_profiling_areas_table() {
  $areas = progressive_profiling_areas_load_all();
  if (!empty($areas)) {
    ctools_include('context');
    $header = array(
      'area_title' => t('Area title'),
      'conditions' => t('Conditions'),
      'status' => t('Status'),
      'actions' => t('Actions'));
    $rows = array();
    $base_admin_url = progressive_profiling_base_admin_url();
    foreach ($areas as $area_id => $area) {
      $row = array();
      $row['area_title'] = $area->area_title;

      // Conditions column.
      $condition_configuration = array();
      if (!empty($area->configuration['conditions'])) {
        $contexts = progressive_profiling_available_contexts();
        foreach ($area->configuration['conditions'] as $condition_key => $plugin_conf) {
          $plugin = ctools_get_access_plugin($plugin_conf['name']);
          $condition_configuration[] = ctools_access_summary($plugin, $contexts, $plugin_conf) . ' ' . l(t('edit condition'), $base_admin_url . '/areas/conditions/' . $area_id . '/edit/' . $condition_key, array('query' => drupal_get_destination()));
        }
        $conditions_logic = !empty($area->configuration['conditions_logic']) ? $area->configuration['conditions_logic'] : 'and';
        $condition_configuration[] = t('<strong>Logic</strong>: @logic', array('@logic' => ($conditions_logic === 'or') ? t('Only one criteria must pass.') : t('All criteria must pass.'))) . ' ' . l(t('edit logic'), $base_admin_url . '/areas/edit/' . $area_id, array('query' => drupal_get_destination()));
      }
      else {
        $condition_configuration[] = t('No conditions defined yet, this area will match any request.');
      }
      // In case we will have some more links, we can just add them to this array.
      // (This is expected maybe for the future.)
      $condition_configuration[] = implode(' | ', array(
        l(t('add condition'), $base_admin_url . '/areas/conditions/' . $area_id . '/add', array('query' => drupal_get_destination())),
      ));
      $row['conf'] = implode('<br />', $condition_configuration);

      // Status column.
      $actions = array();
      if (!empty($area->status)) {
        $row['status'] = '<span style="color: green">' . t('Enabled') . '</span>';
        $actions['status'] = l(t('disable'), $base_admin_url . '/areas/disable/' . $area_id, array('query' => drupal_get_destination()));
      }
      else {
        $row['status'] = '<span style="color: red">' . t('Disabled') . '</span>';
        $actions['status'] = l(t('enable'), $base_admin_url . '/areas/enable/' . $area_id, array('query' => drupal_get_destination()));
      }

      // Actions column.
      $actions['edit'] = l(t('edit'), $base_admin_url . '/areas/edit/' . $area_id, array('query' => drupal_get_destination()));
      $actions['delete'] = l(t('delete'), $base_admin_url . '/areas/delete/' . $area_id, array('query' => drupal_get_destination()));
      $row['actions'] = implode(' | ', $actions);

      $rows[$area_id] = array('data' => $row);
    }
    return array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
    );
  }
  return array();
}

/**
 * Form constructor for the add/edit area form.
 */
function progressive_profiling_areas_add_edit_form($form, &$form_state, $area = NULL) {
  if (empty($area)) {
    $form['area_wrapper'] = array(
      '#type' => 'fieldset',
      '#title' => t('Add area'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
  }
  else {
    $form['area_wrapper'] = array(
      '#type' => 'fieldset',
      '#title' => t('Edit area'),
    );
    $form['area_id'] = array(
      '#type' => 'value',
      '#value' => $area->area_id,
    );
  }
  $area_elements = &$form['area_wrapper'];
  $area_elements['area_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => t('A title used in the administrative interface.'),
    '#required' => TRUE,
    '#default_value' => !empty($area) ? $area->area_title : '',
  );
  $area_elements['logic'] = array(
    '#type' => 'radios',
    '#options' => array(
      'and' => t('All criteria must pass.'),
      'or' => t('Only one criteria must pass.'),
    ),
    '#title' => t('Conditions logic'),
    '#default_value' => !empty($area->configuration['conditions_logic']) ? $area->configuration['conditions_logic'] : 'and',
  );
  $area_elements['submit_button_text'] = array(
    '#type' => 'textfield',
    '#title' => t('Submit button text'),
    '#default_value' => !empty($area->configuration['submit_button_text']) ? $area->configuration['submit_button_text'] : '',
  );
  $area_elements['form_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Form title'),
    '#default_value' => !empty($area->configuration['form_title']) ? $area->configuration['form_title'] : 'To access the content, please fill in the following fields.',
  );
  $area_elements['questions_nr'] = array(
    '#type' => 'textfield',
    '#title' => t('Amount of questions to ask'),
    '#description' => t('Input the amount of questions to ask for this area.'),
    '#default_value' => !empty($area->configuration['questions_nr']) ? $area->configuration['questions_nr'] : 2,
  );
  $area_elements['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => !empty($area) ? t('Update') :t('Add'),
  );
  if (!empty($area)) {
    $area_elements['actions']['cancel'] = array(
      '#type' => 'link',
      '#title' => t('Cancel'),
      '#href' => progressive_profiling_base_admin_url() . '/areas',
    );
  }
  return $form;
}

/**
 * Submit handler for the add/edit area form.
 */
function progressive_profiling_areas_add_edit_form_submit(&$form, &$form_state) {
  if (!empty($form_state['values']['area_id'])) {
    $area = progressive_profiling_area_load($form_state['values']['area_id']);
    $message = t('The area settings have been updated.');
  }
  else {
    $area = new stdClass();
    progressive_profiling_area_object_prepare($area);
    $message = t('The area has been created.');
  }
  $area->area_title = $form_state['values']['area_title'];
  $area->configuration['conditions_logic'] = $form_state['values']['logic'];
  $area->configuration['submit_button_text'] = $form_state['values']['submit_button_text'];
  $area->configuration['questions_nr'] = $form_state['values']['questions_nr'];

  progressive_profiling_area_save($area);
  drupal_set_message($message);
  // By default, redirect to the areas list.
  $form_state['redirect'] = progressive_profiling_base_admin_url() . '/areas';
}

/**
 * Confirmation form for disabling an area.
 *
 * @param object $area
 *  An area object returned by progressive_profiling_area_load().
 */
function progressive_profiling_areas_disable_confirm($form, &$form_state, $area) {
  $form['area_id'] = array(
    '#type' => 'value',
    '#value' => $area->area_id,
  );
  return confirm_form($form, t('Are you sure you want to disable the area?'), progressive_profiling_base_admin_url() . '/areas', t('Are you sure you want to disable the area %area from the progressive profiling?', array('%area' => $area->area_title)));
}

/**
 * Submit handler for the area disable confirm form.
 */
function progressive_profiling_areas_disable_confirm_submit($form, &$form_state) {
  $area = progressive_profiling_area_load($form_state['values']['area_id']);
  $area->status = 0;
  progressive_profiling_area_save($area);
  drupal_set_message(t('The area has been disabled.'));
}

/**
 * Confirmation form for enabling an area.
 *
 * @param object $area
 *  An area object returned by progressive_profiling_area_load().
 */
function progressive_profiling_areas_enable_confirm($form, &$form_state, $area) {
  $form['area_id'] = array(
    '#type' => 'value',
    '#value' => $area->area_id,
  );
  return confirm_form($form, t('Are you sure you want to enable the area?'), progressive_profiling_base_admin_url() . '/areas', t('Are you sure you want to eanble the area %area for the progressive profiling?', array('%area' => $area->area_title)));
}

/**
 * Submit handler for the area enable confirm form.
 */
function progressive_profiling_areas_enable_confirm_submit($form, &$form_state) {
  $area = progressive_profiling_area_load($form_state['values']['area_id']);
  $area->status = 1;
  progressive_profiling_area_save($area);
  drupal_set_message(t('The area has been enabled.'));
}

/**
 * Confirmation form for deleting an area.
 *
 * @param object $area
 *  An area object returned by progressive_profiling_area_load().
 */
function progressive_profiling_areas_delete_confirm($form, &$form_state, $area) {
  $form['area_id'] = array(
    '#type' => 'value',
    '#value' => $area->area_id,
  );
  return confirm_form($form, t('Are you sure you want to remove the area?'), progressive_profiling_base_admin_url() . '/areas', t('Are you sure you want to remove the area %area from the progressive profiling?', array('%area' => $area->area_title)));
}

/**
 * Submit handler for the field enable confirm form.
 */
function progressive_profiling_areas_delete_confirm_submit($form, &$form_state) {
  progressive_profiling_area_delete($form_state['values']['area_id']);
  drupal_set_message(t('The area has been removed.'));
}

/**
 * Page callback for adding a the condition to an area.
 *
 * @param object $area
 *  The profiling area object.
 */
function progressive_profiling_areas_conditions_add($area) {
  ctools_include('context');
  return drupal_get_form('progressive_profiling_areas_condition_form', $area);
}

/**
 * Page callback for editing an area condition.
 *
 * @param object $area
 *  The area object.
 *
 * @param string $condition_key
 *  The identifier of the condition.
 */
function progressive_profiling_areas_conditions_edit($area, $condition_key) {
  ctools_include('context');
  $form_state = array();
  $form_state['build_info']['args'] = array($area);
  // We directly want to show the condition configuration, so we prefill the
  // values from the first step (from progressive_profiling_area_condition_form())
  // into the storage of the form_state.
  $form_state['storage']['condition_configuration'] = TRUE;
  $form_state['storage']['condition_values']['condition_key'] = $condition_key;
  $form_state['storage']['condition_values']['condition_type'] = $area->configuration['conditions'][$condition_key]['name'];
  $form_state['storage']['condition_values']['area_id'] = $area->area_id;

  return drupal_build_form('progressive_profiling_areas_condition_form', $form_state);
}

/**
 * Form constructor for an area field condition.
 *
 * @param object $area
 *  The profiling area object.
 */
function progressive_profiling_areas_condition_form($form, &$form_state, $area) {
  // The calling function must include the ctools contexts.
  // @see progressive_profiling_areas_conditions_add().

  // Check if we should show the condition configuration form.
  if (isset($form_state['storage']['condition_configuration'])) {
    return progressive_profiling_areas_condition_configuration_form($form, $form_state, $area);
  }
  $plugins = ctools_get_relevant_access_plugins(progressive_profiling_available_contexts());
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
  $form['area_id'] = array(
    '#type' => 'value',
    '#value' => $area->area_id,
  );
  $form['actions']['#type'] = 'actions';
  $form['actions']['next'] = array(
    '#type' => 'submit',
    '#value' => t('Next'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => progressive_profiling_base_admin_url() . '/areas',
  );
  $form_state['step'] = 'condition';

  return $form;
}

/**
 * Form constructor for the condition configuration (the second step of the
 * add/edit area condition).
 *
 * @param object $area
 *  The profiling area object.
 *
 * @todo: duplicated code with progressive_profiling_settings_condition_configuration_form ()
 * Should be refactored if possible.
 */
function progressive_profiling_areas_condition_configuration_form($form, &$form_state, $area) {
  $plugin_name = $form_state['storage']['condition_values']['condition_type'];
  $condition_key = !empty($form_state['storage']['condition_values']['condition_key']) ? $form_state['storage']['condition_values']['condition_key'] : NULL;
  $plugin = ctools_get_access_plugin($plugin_name);
  $form['settings'] = array('#tree' => TRUE);
  $form_state['plugin'] = $plugin;

  if (empty($condition_key)) {
    $condition_configuration = array();
  }
  else {
    $condition_configuration = !empty($area->configuration['conditions'][$condition_key]) ? $area->configuration['conditions'][$condition_key] : array();
  }

  $contexts = progressive_profiling_available_contexts();
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
  if (!empty($condition_key)) {
    $form['condition_key'] = array(
      '#type' => 'value',
      '#value' => $condition_key,
    );
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
    '#href' => progressive_profiling_base_admin_url() . '/areas',
  );
  $form_state['step'] = 'condition_configuration';
  return $form;
}

/**
 * Validation handler for the area condition configuration form.
 *
 * Basically, it just forwards the validation to the callback defined by the
 * ctools access plugin.
 */
function progressive_profiling_areas_condition_form_validate($form, &$form_state) {
  if (!empty($form_state['clicked_button']['#remove'])) {
    return;
  }

  if ($form_state['step'] == 'condition_configuration') {
    try {
      // Do some checks before, to be sure we have valid data.
      if (empty($form_state['storage']['condition_values'])) {
        throw new Exception(t('There are no condition available for editing.'));
      }
      if (empty($form_state['storage']['condition_values']['area_id'])) {
        throw new Exception(t('No available area for which to configure the condition.'));
      }
      if (empty($form_state['storage']['condition_values']['condition_type'])) {
        throw new Exception(t('No available condition type to be set.'));
      }
      $area = progressive_profiling_area_load($form_state['storage']['condition_values']['area_id']);
      if (empty($area)) {
        throw new Exception(t('There is no profiling area with the id: @id.', array('@id' => $form_state['storage']['condition_values']['area_id'])));
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
 * Submit handler for the area condition configuration form.
 */
function progressive_profiling_areas_condition_form_submit($form, &$form_state) {
  if ($form_state['step'] == 'condition') {
    $form_state['storage']['condition_values'] = $form_state['values'];
    $form_state['storage']['condition_configuration'] = TRUE;
    $form_state['rebuild'] = TRUE;
  }
  elseif ($form_state['step'] == 'condition_configuration') {
    if ($function = ctools_plugin_get_function($form_state['plugin'], 'settings form submit')) {
      $function($form, $form_state);
    }
    // Just update the configuration for this area condition (or remove it).
    $area = progressive_profiling_area_load($form_state['storage']['condition_values']['area_id']);
    $condition_key = !empty($form_state['values']['condition_key']) ? $form_state['values']['condition_key'] : time();
    if (!empty($form_state['clicked_button']['#remove'])) {
      unset($area->configuration['conditions'][$condition_key]);
      if (empty($area->configuration['conditions'])) {
        unset($area->configuration['conditions']);
      }
    }
    else {
      $area->configuration['conditions'][$condition_key]['settings'] = isset($form_state['values']['settings']) ? $form_state['values']['settings'] : NULL;
      $area->configuration['conditions'][$condition_key]['name'] = $form_state['storage']['condition_values']['condition_type'];
      $area->configuration['conditions'][$condition_key]['not'] = $form_state['values']['not'];
      if (!empty($form_state['values']['context'])) {
        $area->configuration['conditions'][$condition_key]['context'] = $form_state['values']['context'];
      }
    }
    progressive_profiling_area_save($area);
  }
}

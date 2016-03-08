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
  foreach ($fields as $field_name => $field) {
    $row = array();
    // If the field was deleted from the user, we still want to show it, so that
    // the admin can delete if from the progressive profiling.
    // @todo: should we maybe delete the field automatically?
    $row['field_name'] = !empty($field_info[$field_name]) ? $field_info[$field_name]['label'] . ' (' . $field_name . ')' : t('--Missing field--') . ' (' . $field_name . ')';
    $row['conf'] = 'conf';
    $actions = array();
    if (!empty($field->status)) {
      $row['status'] = '<span style="color: green">' . t('Enabled') . '</span>';
      $actions['status'] = l(t('disable'), $base_admin_url . '/fields/disable/' . $field_name, array('query' => drupal_get_destination()));
    }
    else {
      $row['status'] = '<span style="color: red">' . t('Disabled') . '</span>';
      $actions['status'] = l(t('enable'), $base_admin_url . '/fields/enable/' . $field_name, array('query' => drupal_get_destination()));
    }
    // @todo: should we make the actions configurable? They could anyway be
    // altered with a form alter hook.
    $actions['edit'] = l(t('edit'), $base_admin_url . '/fields/edit/' . $field_name, array('query' => drupal_get_destination()));
    $actions['delete'] = l(t('delete'), $base_admin_url . '/fields/delete/' . $field_name, array('query' => drupal_get_destination()));
    $row['actions'] = implode(' | ', $actions);
    $rows[$field_name] = array('data' => $row, 'class' => array('draggable'));
    //$rows[] = $row;
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
  foreach ($form['#rows'] as $field_name => &$value) {
    $form['#rows'][$field_name]['data']['weight'] = drupal_render($form['weight_' . $field_name]);
  }
  $output = theme('table', array('header' => $form['#header'], 'rows' => $form['#rows'], 'attributes' => array('id' => $table_id)));
  $output .= drupal_render_children($form);
  drupal_add_tabledrag($table_id, 'order', 'sibling', 'component-field-weight');
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
  $form['field_name'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#title' => t('Field name'),
    '#description' => t('Please select the field which you want to add to the progressive profiling pool.'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
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
  // @todo: check that the field name is valid.
  $field_name = is_string($field) ? $field : $field->field_name;
  $field_info = field_info_instance('user', $field_name, 'user');
  $form['field_name'] = array(
    '#type' => 'value',
    '#value' => $field_name,
  );
  $lead = is_string($field) ? t('Add field %field_name', array('%field_name' => $field_info['label'])) : t('Edit field %field_name', array('%field_name' => $field_info['label']));
  $form['lead'] = array(
    '#markup' => '<h3>' . $lead . '</h3>',
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
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
  // @todo: Add the submitted field configuration.
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
 * Menu callback for the general progressive profilling settings page.
 */
function progressive_profiling_settings() {
  return array('#markup' => 'settings');
}

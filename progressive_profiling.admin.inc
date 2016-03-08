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
    'questions' => progressive_profiling_fields_table(),
    'add_form' => drupal_get_form('progressive_profiling_add_field_form'),
  );
}

/**
 * Returns all the questions formatted as a table render array.
 */
function progressive_profiling_fields_table() {
  $fields = progressive_profiling_field_load_all();
  $field_info = field_info_instances('user', 'user');
  $header = array(t('Field name'), t('Conditions'), t('Actions'));
  $rows = array();
  foreach ($fields as $field_name => $field) {
    $row = array();
    // If the field was deleted from the user, we still want to show it, so that
    // the admin can delete if from the progressive profiling.
    // @todo: should we maybe delete the field automatically?
    $row[] = !empty($field_info[$field_name]) ? $field_info[$field_name]['label'] . ' (' . $field_name . ')' : t('--Missing field--') . ' (' . $field_name . ')';
    $row[] = 'conf';
    $row[] = 'actions';
    $rows[] = $row;
  }
  return array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
  );
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
 * @param string $field_name
 *  The machine name of the field.
 *
 * @param array $field
 *  The progressive profile field configuration.
 */
function progressive_profiling_fields_add_edit_field_form($form, &$form_state, $field_name, $field = NULL) {
  // @todo: check that the field name is valid.
  $form['field_name'] = array(
    '#type' => 'value',
    '#value' => $field_name,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
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
 * Menu callback for the general progressive profilling settings page.
 */
function progressive_profiling_settings() {
  return array('#markup' => 'settings');
}

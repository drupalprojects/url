<?php

/**
 * Implements hook_field_info().
 */
function url_field_info() {
  $info['url'] = array(
    'label' => t('URL'),
    'description' => t('This field stores a URL with an optional title.'),
    'instance_settings' => array(
      'title_field' => 0,
      'title_fetch' => 0,
    ),
    'default_widget' => 'url_default',
    'default_formatter' => 'url_default',
  );

  return $info;
}

/**
 * Implements hook_field_instance_settings_form().
 */
function url_field_instance_settings_form($field, $instance) {
  $settings = $instance['settings'];

  $form['title_field'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable <em>Title</em> field'),
    '#default_value' => $settings['title_field'],
    '#description' => t('The title attribute is used as a tooltip when the mouse hovers over the image.'),
  );
  $form['title_fetch'] = array(
    '#type' => 'checkbox',
    '#title' => t('Attempt to fetch the title value from the URL if the <em>Title field</em> is empty.'),
    '#default_value' => $settings['title_fetch'],
  );

  return $form;
}

/**
 * Implements hook_field_is_empty().
 */
function url_field_is_empty($item, $field) {
  return !isset($item['value']) || $item['value'] === '';
}

/**
 * Implements hook_field_presave().
 */
function url_field_presave($entity_type, $entity, $field, $instance, $langcode, &$items) {
  $settings = $instance['settings'];

  // If this field is not configured to auto-fetch, stop.
  if (empty($settings['title_fetch'])) {
    return;
  }

  foreach ($items as $delta => $item) {
    if (!empty($item['value']) && empty($item['title'])) {
      // Extract the title from a request to the URL.
      $request = drupal_http_request($item['value']);
      if (empty($request->error) && $request->code == 200 && !empty($request->data)) {
        if (preg_match('!<title>(.*?)</title>!i', $request->data, $matches)) {
          $items[$delta]['title'] = $matches[1];
        }
      }
    }
  }
}

/**
 * Implements hook_field_widget_info().
 */
function url_field_widget_info() {
  $info['url_default'] = array(
    'label' => t('URL field'),
    'field types' => array('url'),
    'settings' => array('size' => 60),
  );

  return $info;
}

/**
 * Implements hook_field_widget_form().
 */
function url_field_widget_form($form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  $settings = $instance['settings'];

  $element['value'] = $element + array(
    '#default_value' => isset($items[$delta]['value']) ? $items[$delta]['value'] : '',
    '#size' => $instance['widget']['settings']['size'],
    '#maxlength' => 3000,
  );

  if (module_exists('elements')) {
    $element['value']['#type'] = 'urlfield';
  }
  else {
    $element['value']['#type'] = 'textfield';
    $element['value']['#element_validate'] = array('url_validate_url');
  }

  if (!empty($settings['title_field'])) {
    $element['title'] = array(
      '#type' => 'textfield',
      '#title' => t('Title'),
      '#default_value' => isset($items[$delta]['title']) ? $items[$delta]['title'] : '',
      '#maxlength' => 1024,
      '#weight' => 10,
    );
  }

  return $element;
}

/**
 * Implements hook_field_formatter_info().
 */
function url_field_formatter_info() {
  $info['url_default'] = array(
    'label' => 'URL',
    'field types' => array('url'),
    'settings' => array(
      'display' => 'title',
      'trim_length' => 80,
    ),
  );

  return $info;
}

/**
 * Implements hook_field_formatter_view().
 */
function url_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  $element = array();
  $intance_settings = $display['settings'];
  $widget_settings = $display['widget']['settings'];

  foreach ($items as $delta => $item) {
    // Parse out query string and fragment from the URL value.
    $options = drupal_parse_url($item['value']);

    $element[$delta] = array(
      '#type' => 'link',
      '#title' => $item['value'],
      '#href' => $options['path'],
      '#options' => array(
        'query' => $options['query'],
        'fragment' => $options['fragment'],
        'attributes'= > isset($item['attributes']) ? $item['attributes'] : array(),
      ),
    );

    $title = &$element[$delta]['#title'];

    // If the title field data is available and it should be displayed, swap
    // the title out after it has been replaced with any tokens.
    if (!empty($item['title']) && $widget_settings['display'] == 'title') {
      // Unsanitizied token replacement here because $options['HTML'] is FALSE
      // by default in theme_link().
      $title = token_replace($item['title'], array($entity_type => $entity), array('sanitize' => FALSE, 'clear' => TRUE));
    }

    // Trim the link to an accepted length.
    if (!empty($widget_settings['trim_length'])) {
      $title = truncate_utf8($title, $widget_settings['trim_length']);
    }
  }

  return $element;
}

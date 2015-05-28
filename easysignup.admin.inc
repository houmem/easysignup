<?php

/**
 * Configure easysignup settings.
 */
function easysignup_admin_settings() {
    $form['email_config'] = array(
      '#type' => 'fieldset',
      '#title' => t('Email settings'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form['email_config']['admin_config'] = array(
      '#type' => 'textfield',
      '#title' => t('Custom sender'),
      '#default_value' => variable_get('easysignup_admin', ''),
      '#description' => t('Enter a name to be used as sender in the headers when sending email with this module. Default value is the site name'),
    );

    $form['email_config']['email_config'] = array(
      '#type' => 'textfield',
      '#title' => t('email'),
      '#default_value' => variable_get('easysignup_mail', ''),
      '#description' => t('Enter a valid email address to be used in the headers when sending email with this module. Default value is the site email'),
    );

    $content_types = _get_content_types();
    if (!empty($content_types)) {
        $checked = variable_get('easysignup_active', array());

        $form['content_config'] = array(
          '#type' => 'fieldset',
          '#title' => t('Content types'),
          '#collapsible' => TRUE,
          '#collapsed' => FALSE,
        );
        foreach ($content_types as $type => $name) {
            $form['content_config'][$type] = array(
              '#type' => 'checkbox',
              '#title' => $name,
              '#default_value' => isset($checked[$type]) ? 1 : 0,
            );

            $form['content_config']['container_|_' . $type] = array(
              '#type' => 'fieldset',
              '#title' => t('Options'),
              '#collapsible' => TRUE,
              '#collapsed' => FALSE,
              '#states' => array(
                'invisible' => array(
                  'input[name=' . $type . ']' => array('checked' => FALSE),
                ),
              ),
            );

            $form['content_config']['container_|_' . $type][$type . '_|_comment'] = array(
              '#type' => 'checkbox',
              '#title' => t('Signup for comments for this content type'),
              '#default_value' => isset($checked[$type]['comment']) ? $checked[$type]['comment'] : 0,
            );
            $form['content_config']['container_|_' . $type][$type . '_|_private'] = array(
              '#type' => 'checkbox',
              '#title' => t('Make this content type private (only logged in users)'),
              '#default_value' => isset($checked[$type]['private']) ? $checked[$type]['private'] : 0,
            );
        }
    }
    else {
        $form['no_content_type'] = array(
          '#type' => 'checkbox',
          '#markup' => t('No content types available')
        );
    }
    $form['#submit'][] = 'easysignup_custom_settings';
    //dpm(get_defined_vars());
    return system_settings_form($form, FALSE);
}

/**
 * callback for easysignup_admin_settings
 * @param type $form
 * @param type $form_state
 */
function easysignup_custom_settings($form, $form_state) {
    $mail = $form_state['values']['email_config'];
    $admin = $form_state['values']['admin_config'];
    //if email field is not empty
    if ($mail !== '' && !valid_email_address($mail)) {
        $mail = $form_state['values']['email_config'];
        form_set_error('email_config', t('The email address is not valid!'));
    }
    else {
        $config = array();
        $content_types = _get_content_types();
        foreach ($content_types as $type => $name) {
            if ($form_state['values'][$type] === 1) {
                if ($form_state['values'][$type . '_|_comment'] === 1) {
                    $config[$type]['comment'] = 1;
                }
                else {
                    $config[$type]['comment'] = 0;
                }

                if ($form_state['values'][$type . '_|_private'] === 1) {
                    $config[$type]['private'] = 1;
                }
                else {
                    $config[$type]['private'] = 0;
                }
            }
        }
        variable_set('easysignup_active', $config);
        variable_set('easysignup_mail', $mail);
        variable_set('easysignup_admin', $admin);
    }
    //dpm(get_defined_vars());
}

<?php

/**
 * generates a hash to be used in the confirmation email
 * @param type $lenth
 * @return string
 */
function _generate_hash() {
  $lenth = 20;
  $model = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
  $hash = '';
  for ($i = 0; $i < $lenth; $i++) {
    $hash .= $model[rand(0, strlen($model) - 1)];
  }
  return $hash;
}

/**
 * mail footer to be used in the mail functions
 * @return string
 */
function _mail_footer($mail) {
  $text = '';
  $hash = db_select('easysignup_users', 'es')
      ->fields('es', array('hash'))
      ->condition('email', $mail)
      ->condition('confirmed', 1)
      ->execute()
      ->fetchField();
  if ($hash !== FALSE) {
    global $base_url;
    $manage = $base_url . '/manage?c=' . $hash;
    $unsubsribe = $base_url . '/unsubsribe?c=' . $hash;
    $text .= "\n\n" . '::::: ' . t('Subscription Management') . ' :::::' . "\n\n";
    $text .= t('You want to customize your subscription') . ': ' . "\n" . $manage . "\n\n";
    $text .= t('You want to unsubscribe') . ': ' . "\n" . $unsubsribe;
    return $text;
  }
  else {
    drupal_set_message(t('This link is not valid'), 'warning');
    drupal_goto('<front>');
  }
}

/**
 * returns the sender name
 * @return string
 */
function _send_from() {
  $stored_name = variable_get('easysignup_admin', '');
  $site_name = variable_get('site_name', '');
  $sent_from = ($stored_name !== '') ? $stored_name : $site_name;
  return $sent_from;
}

/**
 * returns the sender email
 * @return string
 */
function _email_addr() {
  $stored_mail = variable_get('easysignup_mail', '');
  $site_mail = variable_get('site_mail', '');
  $email_addr = ($stored_mail !== '') ? $stored_mail : $site_mail;
  return $email_addr;
}

/**
 * returns the list of content types
 * @return array
 */
function _get_content_types() {
  $output = array();
  $types = node_type_get_types();
  foreach ($types as $type => $obj) {
    $output[$type] = $obj->name;
  }
  return $output;
}

/**
 * check if the submitted email is used
 * @param string $mail
 * @return string or fale
 */
function _subscription_verif($mail) {
  $query = db_select('easysignup_users', 'es')
      ->fields('es', array('email'))
      ->condition('email', $mail)
      ->execute()
      ->fetchField();
  return $query;
}

/**
 * record the user data in the easysignup_users table
 * @param string $mail
 * @param string $hash
 */
function _record_user($mail, $hash, $bundles) {
  $fields = array(
    'email' => $mail,
    'bundles' => $bundles,
    'hash' => $hash,
    'confirmed' => 0,
  );
  db_insert('easysignup_users')->fields($fields)->execute();
}

/**
 * send a confirmation mail to the subscription
 * @param string $to
 * @param string $link
 */
function _send_confirmation_mail($to, $link) {
  $key = 'confirm_subcription';
  $module = 'easysignup';
  $language = language_default();
  $from = _email_addr();
  $params = array(//array pf parameters to be passed to drupal_mail
    'account' => $to,
    'link' => $link
  );
  drupal_mail($module, $key, $to, $language, $params, $from, TRUE);
}

/**
 * verify if the confirmation hash is valid, then set user to confirmed
 * redirect him to front page and set a confirmation message
 */
function _confirmation() {
  $link = drupal_get_query_parameters();
  if (isset($link['c'])) {
    $result = db_select('easysignup_users', 'es')
        ->fields('es', array('hash'))
        ->condition('hash', $link['c'])
        ->condition('confirmed', 0)
        ->execute()
        ->fetchField();
    //if hash is valid
    if ($link['c'] == $result) {
      $site_frontpage = variable_get('site_frontpage', 'node');
      db_update('easysignup_users')
          ->fields(array('confirmed' => 1))
          ->condition('hash', $result)
          ->execute();
      drupal_set_message(t('You will receive a notification email, once new content is published in your chosen categories.'));
      drupal_goto($site_frontpage);
    }
    else {
      drupal_set_message(t('This link is not valid'), 'warning');
      drupal_goto('<front>');
    }
  }
  else {
    drupal_set_message(t('This link is not valid'), 'warning');
    drupal_goto('<front>');
  }//dpm(get_defined_vars());
}

/**
 * calls a form to manage the subscription
 * @return string form to manage the subscription
 */
function _manage() {
  $link = drupal_get_query_parameters();
  if (isset($link['c'])) {
    $hash_var = db_select('easysignup_users', 'es')
        ->fields('es', array('hash'))
        ->condition('hash', $link['c'])
        ->execute()
        ->fetchField();
    if (!empty($hash_var)) {
      return drupal_get_form('_set_manage_form');
    }
    else {
      drupal_set_message(t('This link is not valid'), 'warning');
      drupal_goto('<front>');
    }
  }
}

/**
 * returnsa form to manage the subscription
 * @param type $form
 * @param type $form_state
 * @return string
 */
function _set_manage_form($form, &$form_state) {
  $default = _get_serialized();
  $active = variable_get('easysignup_active', array());
  $form['choose'] = array(
    '#type' => 'fieldset',
    '#title' => t('Your interests'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['choose']['description'] = array(
    '#type' => 'markup',
    '#markup' => t('Update your subscription'),
  );
  foreach ($active as $type => $cnf) {
    $form['choose'][$type] = array(
      '#type' => 'checkbox',
      '#title' => _get_node_name($type),
      '#default_value' => isset($default[$type]) ? 1 : 0,
    );
    $form['choose']['container_|_' . $type] = array(
      '#type' => 'container',
      '#title' => t('Options'),
      '#prefix' => '<div class="sub-container">',
      '#suffix' => '</div>',
    );
    if ($cnf['comment'] == 1) {
      $form['choose']['container_|_' . $type][$type . '_|_comment'] = array(
        '#type' => 'checkbox',
        '#title' => t('Comments'),
        '#default_value' => (isset($default[$type]) && $default[$type]['comment'] == 1) ? 1 : 0,
        '#prefix' => '<div class="sub-comment">',
        '#suffix' => '</div>'
      );
    }
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#prefix' => '<div class="form-actions">',
    '#suffix' => '</div>',
    '#value' => t('Submit'),
  );
  $form['#attributes']['class'][] = 'manage-subscription';
  $form['#submit'][] = '_set_manage_form_submit';
  //dpm(get_defined_vars());
  return $form;
}

/**
 * callback function called when submitting the _set_manage_form
 * @param type $form
 * @param type $form_state
 */
function _set_manage_form_submit(&$form, &$form_state) {
  $abo = array();
  $content_types = variable_get('easysignup_active');
  foreach ($content_types as $type => $opt) {
    if (array_key_exists($type, $form_state['values'])) {
      if ($form_state['values'][$type] == 1) {
        if (isset($form_state['values'][$type . '_|_comment']) &&
            $form_state['values'][$type . '_|_comment'] == 1) {
          $abo[$type]['comment'] = 1;
        }
        else {
          $abo[$type]['comment'] = 0;
        }
      }
    }
  }
  $bundles = serialize($abo);
  _update_user_preferences($bundles);
  //dpm(get_defined_vars());
}

/**
 * record new user preferences
 * @param array $options
 */
function _update_user_preferences($options) {
  $link = drupal_get_query_parameters();
  $hash = _generate_hash();
  $new_values = array(
    'bundles' => $options,
    'hash' => $hash
  );
  db_update('easysignup_users')
      ->fields($new_values)
      ->condition('hash', $link['c'])
      ->execute();
  //dpm(get_defined_vars());
  drupal_set_message(t('The changes has been saved'));
  drupal_goto('<front>');
}

/**
 * load und unserialize the serialized array
 * @return string
 */
function _get_serialized() {
  $link = drupal_get_query_parameters();
  $query = db_select('easysignup_users', 'es')
      ->fields('es', array('bundles'))
      ->condition('hash', $link['c'])
      ->execute()
      ->fetchField();
  $serialized = unserialize($query);
  return $serialized;
}

/**
 * unsubscribe a user
 */
function _unsubsribe() {
  $link = drupal_get_query_parameters();
  if (isset($link['c'])) {
    $hash = db_select('easysignup_users', 'es')
        ->fields('es', array('hash'))
        ->condition('hash', $link['c'])
        ->execute()
        ->fetchField();
    if ($hash != FALSE) {
      db_delete('easysignup_users')->condition('hash', $hash)->execute();
      drupal_set_message(t('You are successfully unsubscribed.'));
      drupal_goto('<front>');
    }
  }
}

/**
 * stores node default status in the "easysignup status" table
 * @param int $nid
 * @return int $status
 */
function _node_status($nid) {
  $status = 0;
  $fetch = db_select('easysignup_nodes', 'es')
      ->fields('es', array('nid'))
      ->condition('nid', $nid)
      ->execute()
      ->fetchField();
  if ($fetch !== FALSE) {
    $status = 1;
  }//dpm(get_defined_vars());
  return $status;
}

/**
 * generates a table of all subscribers
 * @return string
 */
function _subscribers_list() {
  _delete_abo();
  $list = db_select('easysignup_users', 'es')
      ->fields('es', array('email', 'bundles'))
      ->condition('confirmed', 1)
      ->execute()
      ->fetchAll();

  $out = '<table class="abo-list">';
  $out .= '<thead><tr><td><b>' . t('Email') . '</b></td><td><b>' .
      t('Content type') . '</b></td><td><b>' . t('Operation') .
      '</b></td></tr></thead>';
  foreach ($list as $item) {
    $options = unserialize($item->bundles);
    $bundles = _format_bundles($options);
    $path = 'admin/structure/easysignup/subscribers_list';
    $q = array('query' => array('del' => $item->email));
    $delete = l(t('Delete'), $path, $q);

    $out .= '<tr><td>' . $item->email . '</td><td>' . $bundles .
        '</td><td>' . $delete . '</td></tr>';
  }
  $out .= '<table>';
  //dpm(get_defined_vars());
  return $out;
}

/**
 * delete a signed up user bei his email
 */
function _delete_abo() {
  if (user_access('administer easysignup')) {
    $param = drupal_get_query_parameters();
    if (!empty($param)) {
      db_delete('easysignup_users')->condition('email', $param)->execute();
      drupal_set_message(t('Item deleted.'));
      drupal_goto('admin/structure/easysignup/subscribers_list');
    }//dpm(get_defined_vars());
  }
  else {
    drupal_access_denied();
  }//dpm(get_defined_vars());
}

/**
 * send mass mails
 * @global string $base_url
 * @param array $scope
 */
function _send_mass($scope) {
  global $base_url;
  $key = 'send_mass';
  $module = 'easysignup';
  $language = language_default();
  $from = _email_addr();
  $params = array();
  $params['content'] = '';
//
  foreach ($scope as $to => $conf) {
    if (!empty($conf['nodes'])) {
      $params['content'] .= ':::::::::::::::: ' . t('Latest articles') . ' ::::::::::::::::' . "\n\n\n";
      foreach ($conf['nodes'] as $entity => $ids) {
        $params['content'] .= "\n" . '::::: ' . _get_node_name($entity) . ' :::::' . "\n\n";
        foreach ($ids as $n) {
          $node_title = _get_node_title($n) . "\n";
          $params['content'] .= $node_title;
          $params['content'] .= $base_url . '/node/' . $n . "\n\n";
        }
      }
    }
    if (!empty($conf['comments'])) {
      $params['content'] .= ':::::::::::::::::::::::::::::::::::::::::::::::::::' . "\n\n";
      $params['content'] .= "\n" . ':::::::::: ' . t('Latest comments') . '::::::::::' . "\n\n\n";
      foreach ($conf['comments'] as $entity_c => $cids) {
        $params['content'] .= "\n" . '::::: ' . $entity_c . ' :::::' . "\n\n";
        foreach ($cids as $cid => $cnid) {
          $node_title_c = _get_node_title($cnid) . "\n";
          $params['content'] .= $node_title_c;
          $params['content'] .= $base_url . '/node/' . $cnid . '#comment-' . $cid . "\n\n";
        }
      }
    }
    $params['mail_footer'] = _mail_footer($to);
    //send the mail
    drupal_mail($module, $key, $to, $language, $params, $from, TRUE);
    $params['content'] = ''; //empty the old content
  }
  /**
   * @todo delete nodes & comments after sending the mail
   */
  foreach ($scope as $opt) {
    if (isset($opt['nodes'])) {
      foreach ($opt['nodes'] as $node_id) {
        db_update('easysignup_nodes')
            ->fields(array('in_pool' => 0))
            ->condition('nid', $node_id)
            ->execute();
      }
    }
    if (isset($opt['comments'])) {
      foreach ($opt['comments'] as $n_type => $c_params) {
        /* @var $c_params array */
        foreach ($c_params as $comment_id => $c_node_id) {
          db_delete('easysignup_comments')->condition('cid', $comment_id)->execute();
        }
      }
    }
  }//dpm(get_defined_vars());
}

/**
 * build an array of users and theirs a attached nodes & comments
 * @return array
 */
function _mailing_list() {
  $tree = array();
  $user_config = _user_config_load();
  $node_pool = _get_node_pool();
  $comment_pool = _get_comment_pool();
  foreach ($user_config as $u_mail => $active_types) {
    foreach ($active_types as $active_type => $c_status) {
      //add nodes to the tree
      foreach ($node_pool as $n_type => $n_ids) {
        if ($n_type == $active_type) {
          $tree[$u_mail]['nodes'][$n_type] = $n_ids;
        }
      }
      //add comments to the tree
      foreach ($comment_pool as $c_id => $c_params) {
        $loaded_type = _get_node_type($c_params['nid']);
        if ($loaded_type == $active_type) {
          if ($c_status['comment'] == 1) {
            $tree[$u_mail]['comments'][$c_params['name']][$c_id] = $c_params['nid'];
          }
        }
      }
    }
  }//dpm(get_defined_vars());
  return $tree;
}

/**
 * build array of users and their configurations (choises)
 * @return array
 */
function _user_config_load() {
  $user_config = array();
  $assoc_user_bundles = db_select('easysignup_users', 'es')
      ->fields('es', array('email', 'bundles'))
      ->condition('confirmed', 1)
      ->execute()
      ->fetchAll();
  foreach ($assoc_user_bundles as $u) {
    $user_config[$u->email] = unserialize($u->bundles);
  }
  //dpm(get_defined_vars());
  return $user_config;
}

/**
 * build array of nodes ready to be sent
 * @return array
 */
function _get_node_pool() {
  $pool = array();
  $results = db_select('easysignup_nodes', 'e')
      ->fields('e', array('nid', 'type'))
      ->condition('in_pool', 1)
      ->execute()
      ->fetchAll();
  //filter to published nodes
  foreach ($results as $result) {
    $status = _get_node_status($result->nid);
    if ($status == 1) {
      $pool[$result->type][] = $result->nid;
    }
  }//dpm(get_defined_vars());
  return $pool;
}

/**
 * build array of comments ready to be sent
 * @return array
 */
function _get_comment_pool() {
  $pool = array();
  $fetch = db_select('easysignup_comments', 'c')
      ->fields('c', array('cid', 'nid'))
      ->condition('sent', 0)
      ->execute()
      ->fetchAll();
  foreach ($fetch as $row) {
    $name = _get_node_name_from_nid($row->nid);
    $pool[$row->cid]['name'] = $name;
    $pool[$row->cid]['nid'] = $row->nid;
  }
  //dpm(get_defined_vars());
  return $pool;
}

/**
 * get the node status
 * @param int $nid
 * @return int 0 or 1
 */
function _get_node_status($nid) {
  $status = db_select('node', 'n')
      ->fields('n', array('status'))
      ->condition('nid', $nid)
      ->condition('status', 1)
      ->execute()
      ->fetchField();
  return $status;
}

function _get_node_title($nid) {
  return db_select('node', 'n')
          ->fields('n', array('title'))
          ->condition('nid', $nid)
          ->execute()
          ->fetchField();
}

/**
 * find the node name based on the node type
 * @param type string
 * @return string
 */
function _get_node_name($type) {
  $name = db_select('node_type', 'n')
      ->fields('n', array('name'))
      ->condition('type', $type)
      ->execute()
      ->fetchField();
  return $name;
}

/**
 * find the node name based on the node id
 * @param int $nid
 * @return int
 */
function _get_node_name_from_nid($nid) {
  $query = db_select('node', 'n');
  $query->join('node_type', 't', 'n.type = t.type');
  $query->fields('t', array('name'));
  $query->condition('n.nid', $nid);
  $result = $query->execute()->fetchField();
  //dpm(get_defined_vars());
  return $result;
}

/**
 *  return a string of formatted content types
 * @param array $arr
 * @return string
 */
function _format_bundles($arr) {
  $output = '';
  foreach ($arr as $type => $opt) {
    $output .= _get_node_name($type);
    if ($opt['comment'] == 1) {
      $output .= ' (' . t('with comments') . ')';
    }
    $output .= '<br/>';
  }
  return $output;
}

/**
 * check if a node type is activated for notification
 * @param type string
 * @return boolean
 */
function _node_type_is_active($type) {
  $active_nodes = variable_get('easysignup_active', array());
  if (array_key_exists($type, $active_nodes)) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}

/**
 * get node type machine name from nid
 * @param type int
 * @return string
 */
function _get_node_type($nid) {
  $node = node_load($nid);
  $type = $node->type;
  return $type;
}

/**
 * get human readable name of a node type from its machine name
 * @param string $type
 * @return string
 */
function _get_node_name_from_type($type) {
  return db_select('node_type', 't')
          ->fields('t', array('name'))
          ->condition('type', $type)
          ->execute()
          ->fetchField();
}

/**
 * add published comment to the sending pool
 * @param object $comment
 */
function _synchronize_insert_comments($comment) {
  //if comment is published
  if ($comment->status == 1) {
    $node_status = _get_node_status($comment->nid);
    $type = _get_node_type($comment->nid);
    //if node type is active node
    if (_node_type_is_active($type) && $node_status == 1) {
      $fields = array(
        'cid' => $comment->cid,
        'nid' => $comment->nid,
        'sent' => 0,
      );
      db_insert('easysignup_comments')->fields($fields)->execute();
      //dpm(get_defined_vars());
    }
  }
}

/**
 * update the comment status in the sending pool
 * @param object $comment
 */
function _synchronize_update_comments($comment) {
  $cid = $comment->cid;
  $node_status = _get_node_status($comment->nid);
  $type = _get_node_type($comment->nid);
  if (_node_type_is_active($type)) {
    $status = $comment->status;
    //delete disabled comment from the sending pool
    if ($status == 0) {
      db_delete('easysignup_comments')->condition('cid', $cid)->execute();
    }
    if ($status == 1) {
      $is_stored = db_select('easysignup_comments', 'c')
          ->fields('c', array('cid'))
          ->condition('cid', $comment->cid)
          ->execute()
          ->fetchField();
      //if comment is not stored, then store it
      if (!$is_stored && $node_status == 1) {
        $fields = array(
          'cid' => $cid,
          'nid' => $comment->nid,
          'sent' => 0,
        );
        db_insert('easysignup_comments')->fields($fields)->execute();
      }
    }
  }//dpm(get_defined_vars());
}

/**
 * remove deleted comment from the sending pool
 * @param object $comment
 */
function _synchronize_delete_comments($comment) {
  $cid = $comment->cid;
  db_delete('easysignup_comments')->condition('cid', $cid)->execute();
}

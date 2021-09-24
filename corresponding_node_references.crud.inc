<?php
/**
 * @file
 * Include file providing corresponding node reference insert, update, and
 * delete handling.
 */

/**
 * Add any corresponding references on node insertion.
 *
 * @param object $home_node
 *  The node object cached in the node api on this node with a
 *  reference to the away node.
 * @param string $home_field
 *  The field on the home node used to make the reference.
 * @param string $away_node_type
 *  The type of the away node.
 * @param string $away_field
 *  The name of the field on the away node referencing to
 *  the home node.
 */
function corresponding_node_references_insert($home_node, $home_field, $away_node_type, $away_field) {
  $old_node = node_load($home_node->nid, NULL, TRUE);

  // Determine the nodereference values after the insert.
  if (isset($home_node->$home_field) && is_array($home_node->$home_field)) {
    foreach ($home_node->$home_field as  $fields) {
      foreach ($fields as $reference) {
        if (!empty($reference['nid'])) {
          // Load the referenced node if it is of the specified away type.
          // TODO: Do this with EntityFieldQuery
          // See https://api.backdropcms.org/api/backdrop/core%21modules%21node%21node.module/function/node_load_multiple/1
          if ($referenced_node = node_load($reference['nid'], NULL, FALSE)) {
            if ($referenced_node->type == $away_node_type) {
              $away_field_language = field_language('node', $referenced_node, $away_field);
              // Add the new reference.
              // If there are no other references, we need to make sure this
              // is delta 0
              if (array_key_exists($away_field_language, $referenced_node->{$away_field}) == FALSE || $referenced_node->{$away_field}[$away_field_language][0]['nid'] == NULL) {
                $referenced_node->{$away_field}[$away_field_language][0]['nid'] = $home_node->nid;
              }
              else {
                // Add the new reference.
                // Check for doubles, could happen when nodes of same type are
                // referenced.
                $exists = FALSE;
                foreach ($referenced_node->{$away_field}[$away_field_language] as $key => $value) {
                  if ($value['nid'] == $home_node->nid) {
                    $exists = TRUE;
                    break;
                  }
                }

                if (!$exists) {
                  $referenced_node->{$away_field}[$away_field_language][] = array('nid' => $home_node->nid);                }
              }
              _corresponding_node_references_update($referenced_node);
            }
          }
        }
      }
    }
  }
}

/**
 * Change corresponding references on node updating.
 *
 * Corresponding changes are made for any references removed or added.
 *
 * @param object $home_node
 *  The node object cached in the node api on this node with a
 *  reference to the away node.
 * @param string $home_field
 *  The field on the home node used to make the reference.
 * @param string $away_node_type
 *  The type of the away node.
 * @param string $away_field
 *  The name of the field on the away node referencing to
 *  the home node.
 * @param bool $process_unchanged
 *  Whether or not to process node reference fields
 *  whose values have not changed. This is useful for nodes that
 *  existed before installing this module and have corresponding node
 *  reference fields that don't match up.
 */
function corresponding_node_references_update($home_node, $home_field, $away_node_type, $away_field, $process_unchanged = FALSE) {
  // Since home_node is just being saved, $old_node and $home_node are different!
  $old_node = !empty($home_node->original) ? $home_node->original : $home_node;
  $old = $new = array();

  // Determine the nodereference values before the update.
  if (isset($old_node->$home_field) && is_array($old_node->$home_field)) {
    foreach ($old_node->$home_field as $lang => $fields) {
      foreach ($fields as $reference) {
        if (!empty($reference['nid'])) {
          $old[] = $reference['nid'];
        }
      }
    }
  }

  // If we are processing unchanged references, remove all new references
  // from the old references.
  if ($process_unchanged) {
    $old  = array_diff($old, $new);
  }

  // Determine the nodereference values after the update.
  if (isset($home_node->$home_field) && is_array($home_node->$home_field)) {
    foreach ($home_node->$home_field as $lang => $fields) {
      foreach ($fields as $reference) {
        if (!empty($reference['nid'])) {
          $new[] = $reference['nid'];
        }
      }
    }
  }

  // Handle removed references.
  if ( !empty($old) ) {
    foreach ($old as $data) {
      if ($removed = array_diff($old, $new)) {
        foreach ($removed as $nid) {
          // Load the referenced node if it is of the specified away type.
          if ($referenced_node = node_load($nid, NULL, FALSE)) {
            // Self-references are handled by the node_reference module anyway.
            if ($referenced_node->type == $away_node_type && $nid != $home_node->nid) {
              $away_field_language = field_language('node', $referenced_node, $away_field);
              if (isset($referenced_node->{$away_field}[$away_field_language]) && is_array($referenced_node->{$away_field}[$away_field_language])) {
                // Iterate through the away node's references.
                foreach ($referenced_node->{$away_field}[$away_field_language] as $key => $value) {
                  // Remove references to the deleted node.
                  if ($value['nid'] && $value['nid'] == $home_node->nid) {
                    unset($referenced_node->{$away_field}[$away_field_language][$key]);
                    _corresponding_node_references_update($referenced_node);
                    break;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  // Handle added references.
  // No array diff: a reference overload or a mass update could have happened.
  if ($added = $new) {
    foreach ($added as $nid) {
      // Load the referenced node if it is of the specified away type.
      if ($referenced_node = node_load($nid, NULL, FALSE)) {
        $away_field_language = field_language('node', $referenced_node, $away_field);
        // Self-references are handled by the node_reference module anyway.
        if ($referenced_node->type == $away_node_type && $nid != $home_node->nid) {
          // Detect whether the reference already exists.
          $exists = FALSE;
          if (isset($referenced_node->{$away_field}[$away_field_language]) && !empty($referenced_node->{$away_field}[$away_field_language])) {
            foreach ($referenced_node->{$away_field}[$away_field_language] as $data) {
              if ($data['nid'] == $home_node->nid) {
                $exists = TRUE;
                break;
              }
            }
          }

          // Empty places are removed.
          // Yes this means the deltas change on the away node when a
          // reference is made on the home node.
          $values = array();
          if (isset($referenced_node->{$away_field}[$away_field_language])) {
            foreach ($referenced_node->{$away_field}[$away_field_language] as $value) {
              if (!empty($value['nid'])) {
                $values[] = $value;
              }
            }
          }
          $referenced_node->{$away_field}[$away_field_language] = $values;
          // Add the new reference. Don't create a duplicate.
          if (!$exists) {
            // Get the allowed values.
            $unlimited = FALSE;
            $field = field_info_field($away_field);
            if ($field) {
              if ($field['cardinality'] == -1) {
                $unlimited = TRUE;
                $allowed_references = 0;
              }
              else {
                $allowed_references = $field['cardinality'];
              }
              // Check for reference overloading.
              $references = count($referenced_node->{$away_field}[$away_field_language]) + 1;
              if (($allowed_references >= $references) || $unlimited) {
                $referenced_node->{$away_field}[$away_field_language][] = array('nid' => $home_node->nid);
                _corresponding_node_references_update($referenced_node);
              }
              else {
                $t_reference = format_plural($references, '1 reference', '@count references');
                $t_allowed = format_plural($allowed_references, '1 reference is', '@count references are');
                backdrop_set_message(
                  t('Unable to update the corresponding reference due to reference overloading. This means item @title would have  @t_reference and only @t_allowed permitted. There are two options: either <a href="@url">manually update</a> the existing reference on item @title; or update the field settings for the <a href="@content_type_url">@content_type content type</a> to allow the reference instance to have more references.',
                    array(
                      '@title' => $referenced_node->title,
                      '@url' => url('node/' . $referenced_node->nid . '/edit'),
                      '@t_reference' => $t_reference,
                      '@t_allowed' => $t_allowed,
                      '@content_type' => node_type_get_name($referenced_node),
                      '@content_type_url' => url('admin/structure/types/manage/' . $referenced_node->type),
                    )
                  ),
                  'error'
                );
              }
            }
          }
        }
      }
    }
  }
}

/**
 * Remove corresponding references on node deletion.
 *
 * @param object $home_node
 *  The node object cached in the node api on this node with a
 *  reference to the away node.
 * @param string $home_field
 *  The field on the home node used to make the reference.
 * @param string $away_node_type
 *  The type of the away node.
 * @param string $away_field
 *  The name of the field on the away node referencing to
 *  the home node.
 */
function corresponding_node_references_delete($home_node, $home_field, $away_node_type, $away_field) {
  // Iterate through the field's references.
  foreach ($home_node->$home_field as $lang => $langdata) {
    foreach ($langdata as $reference) {
      if (!empty($reference['nid'])) {
        // Load the referenced node if it is of the specified away type.
        if ($referenced_node = node_load($reference['nid'], NULL, FALSE)) {
          if ($referenced_node->type == $away_node_type) {
            // Iterate through the away node's references.
            foreach ($referenced_node->{$away_field}[$lang] as $key => $value) {
              // Remove references to the deleted node.
              if ($value['nid'] && $value['nid'] == $home_node->nid) {
                unset($referenced_node->{$away_field}[$lang][$key]);
                _corresponding_node_references_update($referenced_node);
                break;
              }
            }
          }
        }
      }
    }
  }
}

/**
 * Update field data.
 *
 * @param object $node
 *  The referenced node to be updated.
 */
function _corresponding_node_references_update(&$node) {
  field_attach_presave('node', $node);
  field_attach_update('node', $node);
  // Flush Entity Cache record.
  cache_clear_all($node->nid, 'cache_entity_node');
}

<?php

/**
 * @file
 * This module implements the integration between LUSK Working Paper and RePEc.
 * Case 39998.
 */

/**
 * This function creates directory based on the settings informed by the user.
 */
function repec_create_directory() {
  $repec_papers_path = variable_get('repec_base_path') . '/' . variable_get('repec_archive_code') . '/' . variable_get('repec_paper_dir', 'wpaper') . '/';

  if (!file_exists(variable_get('repec_base_path'))) {
    if (!mkdir(variable_get('repec_base_path'), 0755, TRUE)) {
      drupal_set_message(t("Directory couldn't be created in this path"), 'error');
    }
  }

  // Remove all files of type .rdf
  $files = glob($repec_papers_path . '/*.rdf');
  foreach ($files as $file) { // iterate files
    if (is_file($file)) {
      unlink($file); // delete file
    }
  }

  // Generate the directory and the archive and series templates
  repec_generate_archive_template();
  repec_generate_series_template();

  // get all contents of type Working Paper
  $type = "working_paper";
  $nodes = node_load_multiple(array(), array('type' => $type));

  // save all papers again so that the files can be generated.
  foreach ($nodes as $paper) {
    repec_generate_paper_template($paper);
  }
}

/**
 * This function generates the repec template for the article.
 * Based on this url: http://ideas.repec.org/t/papertemplate.html
 * This function returns the path for the template generated
 */
function repec_generate_paper_template($node) {
  $repec_papers_path = variable_get('repec_base_path') . '/' . variable_get('repec_archive_code') . '/' . variable_get('repec_paper_dir', 'wpaper') . '/';
  $content_type = variable_get('repec_paper_content_type');
  $series_name = variable_get('repec_series_name');

  if ($node->type == $content_type) {
    $file_name = variable_get('repec_paper_dir', 'wpaper') . '_' . $node->vid . '.rdf';

    if (!file_exists($repec_papers_path)) {
      if (!mkdir($repec_papers_path, 0755, TRUE)) {
        drupal_set_message(t("Directory couldn't be created in this path"), 'error');
      }
    }

    $content = 'Template-Type: ReDIF-Paper 1.0';
    $content .= "\n";

    $node_vars = get_object_vars($node);

    $author_field = repec_get_field_content_type($content_type, $series_name, 'author');
    if (!empty($author_field) && !empty($node_vars[$author_field]['und'][0]['value'])) {
      $authors = repec_generate_array_authors($node_vars[$author_field]['und'][0]['value']);
      foreach ($authors as $author) {
        $content .= 'Author-Name: ' . $author;
        $content .= "\n";
      }
    }

    // Title of the Article
    $content .= 'Title: ' . $node->title;
    $content .= "\n";

    // Abstract of the article
    $abstract = repec_get_field_content_type($content_type, $series_name, 'abstract');
    if (!empty($abstract) && !empty($node_vars[$abstract]['und'][0]['value'])) {
      $content .= 'Abstract: ' . str_replace(array("\r", "\n", "<br />", "<br/>"), "", $node_vars[$abstract]['und'][0]['value']);
      $content .= "\n";
    }

    // Date of the article
    $create_date = repec_get_field_content_type($content_type, $series_name, 'create_date');
    if (!empty($create_date) && !empty($node_vars[$create_date]['und'][0]['value'])) {
      $content .= 'Creation-Date: ' . $node_vars[$create_date]['und'][0]['value'];
      $content .= "\n";
    }

    // File for the article
    $file_url = repec_get_field_content_type($content_type, $series_name, 'file_url');
    if (!empty($file_url) && !empty($node_vars[$file_url]['und'][0]['fid'])) {
      $file_article = file_load($node_vars[$file_url]['und'][0]['fid']);
      // File URL.
      $content .= 'File-URL: ' . str_replace(" ", "%20", file_create_url($file_article->uri));
      $content .= "\n";
      // File Format.
      $content .= 'File-Format: ' . ucfirst($file_article->filemime);
      $content .= "\n";
    }

    // Keywords
    $key = repec_get_field_content_type($content_type, $series_name, 'keywords');
    if (!empty($key) && !empty($node_vars[$key]['und'])) {
      $keywords = '';
      foreach ($node_vars[$key]['und'] as $keyword) {
        $term = taxonomy_term_load($keyword['tid']);
        $keywords .= $term->name . ', ';
      }
      if (strlen($keywords) > 0) {
        $keywords = substr($keywords, 0, strlen($keywords)-2);
      }
      $content .= 'Keywords: ' . $keywords;
      $content .= "\n";
    }

    // Number or id of the Article
    $content .= 'Number: ' . $node->vid;
    $content .= "\n";
    // Handle that uniquely identifies the paper.
    $content .= 'Handle: RePEc:' . variable_get('repec_archive_code') . ':wpaper:' . $node->vid;
    $content .= "\n";

    if (!file_put_contents($repec_papers_path . $file_name, $content)) {
      drupal_set_message(t("File couldn't be created"), "error");
    }

    return $repec_papers_path . $file_name;
  }
}

/**
 * Remove a template file
 */
function repec_remove_paper_template($node) {
  $repec_papers_path = variable_get('repec_base_path') . '/' . variable_get('repec_archive_code') . '/' . variable_get('repec_paper_dir', 'wpaper') . '/';
  $file_name = variable_get('repec_paper_dir', 'wpaper') . '_' . $node->vid . '.rdf';
  unlink($repec_papers_path . $file_name);
}

/**
 * This function generates the repec template for the series.
 * Based on this url: http://ideas.repec.org/t/seritemplate.html
 * This function returns the path for the template generated
 */
function repec_generate_series_template() {
  $repec_archive_path = variable_get('repec_base_path') . '/' . variable_get('repec_archive_code') . '/';
  if (!file_exists($repec_archive_path)) {
    if (!mkdir($repec_archive_path, 0777, TRUE)) {
      drupal_set_message(t("Directory couldn't be created in this path"), 'error');
    }
  }

  $file_name = variable_get('repec_archive_code') . 'seri.rdf';

  $content = 'Template-type: ReDIF-Series 1.0';
  $content .= "\n";
  $content .= 'Name: ' . variable_get('repec_paper_name');
  $content .= "\n";
  $content .= 'Provider-Name: ' . variable_get('repec_provider_name');
  $content .= "\n";
  $content .= 'Provider-Homepage: ' . variable_get('repec_provider_homepage');
  $content .= "\n";
  $content .= 'Provider-Institution: ' . variable_get('repec_provider_institution');
  $content .= "\n";
  $content .= 'Maintainer-Name: ' . variable_get('repec_maintainer_name');
  $content .= "\n";
  $content .= 'Maintainer-Email: ' . variable_get('repec_maintainer_email');
  $content .= "\n";
  $content .= 'Type: ReDIF-Paper';
  $content .= "\n";
  $content .= 'Handle: RePEc:' . variable_get('repec_archive_code') . ':wpaper';
  $content .= "\n";

  if (!file_put_contents($repec_archive_path . $file_name, $content)) {
    drupal_set_message(t("File couldn't be created"), "error");
  }
  return $repec_archive_path . $file_name;
}

/**
 * This function generates the repec template for the series.
 * Based on this url: http://ideas.repec.org/t/archtemplate.html
 * This function returns the path for the template generated
 */
function repec_generate_archive_template() {
  $repec_archive_path = variable_get('repec_base_path') . '/' . variable_get('repec_archive_code') . '/';
  if (!file_exists($repec_archive_path)) {
    if (!mkdir($repec_archive_path, 0777, TRUE)) {
      drupal_set_message(t("Directory couldn't be created in this path"), 'error');
    }
  }

  $file_name = variable_get('repec_archive_code') . 'arch.rdf';

  $content = 'Template-type: ReDIF-Archive 1.0';
  $content .= "\n";
  $content .= 'Handle: RePEc:' . variable_get('repec_archive_code');
  $content .= "\n";
  $content .= 'Name: ' . variable_get('repec_provider_name');
  $content .= "\n";
  $content .= 'Maintainer-Name: ' . variable_get('repec_maintainer_name');
  $content .= "\n";
  $content .= 'Maintainer-Email: ' . variable_get('repec_maintainer_email');
  $content .= "\n";
  $content .= 'Description: This archive collects ' . variable_get('repec_paper_name') . ' from ' . variable_get('repec_provider_name');
  $content .= "\n";
  $content .= 'URL: ' . variable_get('repec_provider_homepage') . variable_get('repec_base_path') . '/' . variable_get('repec_archive_code') . '/';
  $content .= "\n";

  if (!file_put_contents($repec_archive_path . $file_name, $content)) {
    drupal_set_message(t("File couldn't be created"), "error");
  }
  return $repec_archive_path . $file_name;
}

/**
 * This generates an array of authors if they are written in a textfield separated by delimiters (and, &, ',', ';', with).
 */
function repec_generate_array_authors($authors_names) {
  // Author Name
  // Break by and
  $check_delimiters = variable_get('repec_check_delimiters');
  if (empty($check_delimiters) || $check_delimiters == 0) {
    return array($authors_names);
  }

  $authors_temp = explode(" and ", $authors_names);

  // Break by &
  $authors = array();
  foreach ($authors_temp as $a) {
    $authors = array_merge($authors, explode("&", $a));
  }

  // Break by ,
  $authors_temp = $authors;
  $authors = array();
  foreach ($authors_temp as $a) {
    $authors = array_merge($authors, explode(",", $a));
  }

  // Break by ;
  $authors_temp = $authors;
  $authors = array();
  foreach ($authors_temp as $a) {
    $authors = array_merge($authors, explode(";", $a));
  }

  // Break by with
  $authors_temp = $authors;
  $authors = array();
  foreach ($authors_temp as $a) {
    $authors = array_merge($authors, explode("with", $a));
  }

  $authors_temp = $authors;
  $authors = array();
  foreach ($authors_temp as $a) {
    $a = trim($a);
    if ($a != NULL && $a != '') {
      $authors[] = $a;
    }
  }
  return $authors;
}

/**
 * This function associates content type and series fields.
 */
function repec_add_association($content_type, $series_name, $field_content_type, $field_series) {
  db_insert('repec_assoc')
    ->fields(array(
      'content_type' => $content_type,
      'series_name' => $series_name,
      'field_content_type' => $field_content_type,
      'field_series' => $field_series,
      'enabled' => 0
    ))
    ->execute();
}

/**
 * This function disaassociates content type and series fields.
 */
function repec_delete_association($content_type, $series_name, $field_content_type = NULL, $field_series = NULL) {

  $query = db_delete('repec_assoc')
    ->condition('content_type', $content_type)
    ->condition('series_name', $series_name);

  if ($field_content_type != NULL) {
    $query->condition('field_content_type', $field_content_type);
  }
  if ($field_series != NULL) {
    $query->condition('field_series', $field_series);
  }

  $query->execute();
}

/**
 * Returns the associated field_series
 */
function repec_get_field_series($content_type, $series_name, $field_content_type) {
  $row = db_select('repec_assoc', 'ra')
    ->fields('ra')
    ->condition('content_type', $content_type)
    ->condition('series_name', $series_name)
    ->condition('field_content_type', $field_content_type)
    ->execute()
    ->fetchObject();
  return $row->field_series;
}

/**
 * Returns the associated field of the content type
 */
function repec_get_field_content_type($content_type, $series_name, $field_series) {
  $row = db_select('repec_assoc', 'ra')
    ->fields('ra')
    ->condition('content_type', $content_type)
    ->condition('series_name', $series_name)
    ->condition('field_series', $field_series)
    ->execute()
    ->fetchObject();
  return $row->field_content_type;
}

/**
 * This enables the association for the informed content type.
 */
function repec_enable_association($content_type, $series_name) {
  db_update('repec_assoc')
    ->fields(array(
      'enabled' => 0
    ))
    ->condition('series_name', $series_name)
    ->execute();

  db_update('repec_assoc')
    ->fields(array(
      'enabled' => 1
    ))
    ->condition('content_type', $content_type)
    ->condition('series_name', $series_name)
    ->execute();
}

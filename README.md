Corresponding Node References
=============================

Syncs the node reference between two content types which have a nodereference to each
other, so double editing nodes is no more needed. If one node has a reference the
other node receives also a reference to the saved node if it is referenced in that node.

Example
-------

Content type "Page" has a node reference to content type "Section". And Section has a node
reference to Page. The user creates content "My first page" of type Page and references it to
content "Section A" of type Section. "Section A" will receive an update in its node reference
field pointing back to "My first page".

Requirements
------------

Node reference as part of the References module.

Installation
------------

* Install this module using the official Backdrop CMS instructions at
  <https://backdropcms.org/guide/modules>
* Configure at admin/config/content/corresponding_node_references

Configuration
-------------

* Create two content types.
* Create a node reference field on the first content type pointing to the second.
  And vice-versa.
* Go to Administration » Configuration » System » Corresponding Node References.
* Select to enable the corresponding referencing for these content types pointing to each other.
* Create a piece of content and reference it to another of the second type. The
  second piece of content will automatically be updated with a reference to the first.
* Content can also be batch updated by going to Administration » Configuration » System
  » Corresponding Node References » Update Existing Nodes.

Issues
------

Go to the [issues page](https://github.com/backdrop-contrib/corresponding_node_references/issues)
submit bug reports and feature suggestions, or to track changes.

Current Maintainers
-------------------

* [Herb v/d Dool](https://github.com/herbdool)
* Seeking additional maintainers.

Credits
-------

* Ported to Backdrop by [Herb v/d Dool](https://github.com/herbdool/) from the
  Drupal 7 module [CNR](https://www.drupal.org/project/cnr).
* Originally developed for Drupal by [Dominique De Cooman](https://www.drupal.org/u/domidc)

License
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.

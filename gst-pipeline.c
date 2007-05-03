/*
 * Copyright (C) 2007 daniel g. siegel <dgsiegel@gmail.com>
 * All rights reserved. This software is released under the GPL2 licence.
 */

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>

#include "cheese.h"
#include "gst-pipeline.h"


void gst_set_play(GstElement *pipeline) {
  gst_element_set_state(pipeline , GST_STATE_PLAYING);
}

void gst_set_stop(GstElement *pipeline) {
  gst_element_set_state(pipeline , GST_STATE_NULL);
}


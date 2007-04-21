#!/usr/bin/python2.4
## -*- coding: utf-8 -*-

# Copyright (c) 2007 Raphael Slinckx <raphael@slinckx.net>
# All rights reserved. This software is released under the GPL2 licence.


import gobject
gobject.threads_init()
import gtk, gtk.glade, gtk.gdk
gtk.gdk.threads_init()
import pygst
import gnome, gnomevfs
import gst
import time
import Image
import os, glob
from os.path import join, dirname, exists, abspath

GLADE = gtk.glade.XML(join(dirname(__file__), "camera.glade"))
PHOTOBOOTH_DIR = join(dirname(__file__), "photobooth")
try:
	os.mkdir(PHOTOBOOTH_DIR)
except:
	pass

WIDTH, HEIGHT = 640, 480
THUMB_WIDTH, THUMB_HEIGHT = WIDTH/5, HEIGHT/5
EFFECTS = {
"Quark": ("quarktv",),
"Rev": ("revtv",),
"Vertigo": ("vertigotv",),
"Shagadelic": ("shagadelictv",),
"Warp": ("warptv",),
"Dice": ("dicetv",),
#"Aging": ("agingtv",),
"Edge": ("edgetv",),
"Shrek": ("videobalance", {"saturation": 0.5, "hue": -0.5}),
"Désaturé": ("videobalance", {"saturation": 0.5}),
"Hulk": ("videobalance", {"saturation": 1.5, "hue": -0.5}),
"Mauve": ("videobalance", {"saturation": 1.5, "hue": +0.5}),
"Triste": ("videobalance", {"saturation": 0.5, "contrast": 2}),
"Saturation": ("videobalance", {"saturation": 2, "contrast": 2}),
"Noir/Blanc": ("videobalance", {"saturation": 0}),
}


class PhotoboothEngine(gobject.GObject):
	__gsignals__ = {
		"photo-taken" : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, [gobject.TYPE_STRING]),
	}
	def __init__(self):
		gobject.GObject.__init__(self)

		self.pipeline = gst.Pipeline("photobooth")
		self.do_grab_frame = False
		self.xwindow = 0

		# Alternate sources to run outside of Internet Tablet
		#source = gst.element_factory_make("videotestsrc", "src")
		#source.set_property("is-live", True)
		source = gst.element_factory_make("gconfvideosrc", "src")
		self.pipeline.add(source)

		self.csp1 = gst.element_factory_make("ffmpegcolorspace", "csp1")
		self.pipeline.add(self.csp1)
		
		self.effect = gst.element_factory_make(EFFECTS[EFFECTS.keys()[0]][0], "effect")
		self.pipeline.add(self.effect)
		
		self.csp2 = gst.element_factory_make("ffmpegcolorspace", "csp2")
		self.pipeline.add(self.csp2)
		
		caps1 = gst.element_factory_make("capsfilter", "caps1")
		self.pipeline.add(caps1)
		
		tee = gst.element_factory_make("tee", "tee")
		self.pipeline.add(tee)
		
		queuevid = gst.element_factory_make("queue", "queuevid")
		self.pipeline.add(queuevid)
		
		csp3 = gst.element_factory_make("ffmpegcolorspace", "csp3")
		self.pipeline.add(csp3)

		flip = gst.element_factory_make("videoflip", "flip")
		flip.set_property("method", 4)
		self.pipeline.add(flip)
		
		self.sink = gst.element_factory_make("xvimagesink", "sink")
		self.pipeline.add(self.sink)
		
		queueimg = gst.element_factory_make("queue", "queueimg")
		self.pipeline.add(queueimg)
		
		fakesink = gst.element_factory_make("fakesink", "fakesink")
		self.pipeline.add(fakesink)
		
		fakesink.set_property("signal-handoffs", True)
		fakesink.connect("handoff", self.on_buffer)
		caps1.set_property('caps', gst.caps_from_string("video/x-raw-rgb,width=%d,height=%d,bpp=24,depth=24,framerate=30/1" % (WIDTH, HEIGHT)))
		
		source.link(self.csp1)
		self.csp1.link(self.effect)
		self.effect.link(self.csp2)
		self.csp2.link(caps1)
		caps1.link(tee)
		
		tee.link(queuevid)
		queuevid.link(csp3)
		csp3.link(flip)
		flip.link(self.sink)
		
		tee.link(queueimg)
		queueimg.link(fakesink)

	def on_buffer(self, element, buffer, pad):
		if self.do_grab_frame:
			self.save_image(buffer)
			self.do_grab_frame = False
		return True

	def save_image(self, buffer):
		img = Image.frombuffer("RGB", (WIDTH, HEIGHT), buffer, 'raw', 'RGB', 0, 1)
		
		i = 0
		f = join(PHOTOBOOTH_DIR, "img-%02d.jpg" % i)
		while exists(f):
			i += 1
			f = join(PHOTOBOOTH_DIR, "img-%02d.jpg" % i)
		img.save(f)
		self.emit("photo-taken", f)
		print '...Saved: %s' % f

	def stop(self):
		self.pipeline.set_state(gst.STATE_NULL)

	def pause(self):
		self.pipeline.set_state(gst.STATE_PAUSED)

	def play(self):
		self.pipeline.set_state(gst.STATE_PLAYING)
	
	def set_window(self, window):
		self.xwindow = window
		self.sink.set_xwindow_id(window)
	
	def grab_frame(self):
		self.do_grab_frame = True
	
	def set_effect(self, effect, params={}):
		self.stop()

		self.csp1.unlink(self.effect)
		self.effect.unlink(self.csp2)
		self.pipeline.remove(self.effect)
		
		print 'Creating effect:', effect, params
		self.effect = gst.element_factory_make(effect, "effect")
		for key, val in params.items():
			self.effect.set_property(key, val)

		self.pipeline.add(self.effect)
		self.csp1.link(self.effect)
		self.effect.link(self.csp2)
		
		self.play()

		self.set_window(self.xwindow)

class PhotoboothModel(gtk.ListStore):
	def __init__(self, engine):
		gtk.ListStore.__init__(self, gtk.gdk.Pixbuf, str)
		self.engine = engine
		self.engine.connect("photo-taken", self.on_photo_taken)
		self.load_existing()
	
	def append_photo(self, f):
		pix = gtk.gdk.pixbuf_new_from_file(f)
		pix = pix.scale_simple(THUMB_WIDTH, THUMB_HEIGHT, gtk.gdk.INTERP_BILINEAR)
		self.append([pix, f])

	def load_existing(self):
		for f in glob.glob(join(PHOTOBOOTH_DIR, "*.jpg")):
			self.append_photo(f)
	
	def on_photo_taken(self, engine, f):
		self.append_photo(f)

class PhotoboothWindow:
	def __init__(self, engine):
		self.engine = engine
		self.window = GLADE.get_widget("photobooth")
		self.timer_id = 0

		self.window.connect("delete_event", self.on_delete_event)
		self.window.connect("destroy", self.on_destroy)

		self.screen = GLADE.get_widget("screen")
		self.screen.set_size_request(500, 380)
		self.screen.add_events(gtk.gdk.BUTTON_PRESS_MASK)
		self.screen.connect("expose-event", self.on_expose);
		
		self.take_picture = GLADE.get_widget("take_picture")
		self.take_picture.connect("clicked", self.on_take_picture)

		self.status = GLADE.get_widget("status")

		self.previews = GLADE.get_widget("previews")
		self.preview_model = PhotoboothModel(self.engine)
		self.previews.connect("item-activated", self.on_photo_activated)
		self.previews.set_model(self.preview_model)
		self.previews.set_property("columns", 1000000)
		self.previews.set_property("pixbuf-column", 0)
		self.previews.set_size_request(WIDTH, THUMB_HEIGHT)

		self.effects = GLADE.get_widget("effects")
		for effect in EFFECTS.keys():
			self.effects.append_text(effect)
		self.effects.connect("changed", self.on_effects_changed)
		self.effects.set_active(0)

		self.engine.connect("photo-taken", self.on_photo_taken)
		self.on_photo_taken(self.engine, None)

		self.window.show_all()
		self.status.hide()

	def on_delete_event(self, widget, event):
		return False

	def on_destroy(self, widget):
		engine.stop()
		gtk.main_quit()

	def on_expose(self, dummy1, dummy2):
		self.engine.set_window(self.screen.window.xid)
	
	def on_take_picture(self, button):
		print 'Smile !'
		if self.timer_id != 0:
			return

		def foobar():
			self.status.set_markup("<b>...Smile!</b>")
			self.engine.grab_frame()

		self.take_picture.hide()
		self.status.show()
		self.status.set_markup("<b>3...</b>")

		gobject.timeout_add(1000, lambda: self.status.set_markup("<b>...2...</b>"))
		gobject.timeout_add(2000, lambda: self.status.set_markup("<b>...1...</b>"))
		self.timer_id = gobject.timeout_add(3000, foobar)
	
	def on_photo_taken(self, engine, f):
		def foobar():
			if len(self.preview_model) == 0:
				return

			iter = self.preview_model.get_iter_from_string(str(len(self.preview_model)-1))
			path = self.preview_model.get_path(iter)
			self.previews.scroll_to_path(path, True, 1.0, 0.5)

			self.timer_id = 0
			self.status.hide()
			self.take_picture.show()
		gobject.idle_add(foobar)

	def on_effects_changed(self, combo):
		effect_desc = EFFECTS[self.effects.get_active_text()]
		self.engine.set_effect(*effect_desc)
	
	def on_photo_activated(self, effects, path):
		gnome.url_show("file://%s" % abspath(self.preview_model[path][1]))

class PhotoboothUploader:
	def __init__(self, engine):
		self.engine = engine
		self.engine.connect("photo-taken", self.on_photo_taken)
	
	def on_photo_taken(self, engine, f):
		def foobar():
			os.system("./photobooth-upload '%s' &" % abspath(f))
		gobject.idle_add(foobar)

engine = PhotoboothEngine()
window = PhotoboothWindow(engine)
uploader = PhotoboothUploader(engine)

engine.play()

gtk.main()

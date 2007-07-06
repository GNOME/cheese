#ifndef __PIPELINE_PHOTO_H__
#define __PIPELINE_PHOTO_H__
#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define PIPELINE_TYPE             (pipeline_get_type ())
#define PIPELINE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PIPELINE_TYPE, Pipeline))
#define PIPELINE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), PIPELINE_TYPE, PipelineClass))
#define IS_PIPELINE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PIPELINE_TYPE))
#define IS_PIPELINE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), PIPELINE_TYPE))
#define PIPELINE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), PIPELINE_TYPE, PipelineClass))

typedef struct _Pipeline Pipeline;
typedef struct _PipelineClass PipelineClass;

struct _Pipeline 
{
	GObject parent;

  GstElement *pipeline;
  GstElement *ximagesink;
  GstElement *fakesink;
};

struct _PipelineClass 
{
	GObjectClass parent_class;
};

Pipeline*     pipeline_new                 (void);
GType         pipeline_get_type            (void);
void          pipeline_set_play            (Pipeline *self);
void          pipeline_set_stop            (Pipeline *self);
void          pipeline_create              (Pipeline *self);
GstElement   *pipeline_get_ximagesink      (Pipeline *self);
GstElement   *pipeline_get_fakesink        (Pipeline *self);
GstElement   *pipeline_get_pipeline        (Pipeline *self);
void          pipeline_button_clicked      (GtkWidget *widget, gpointer self);
void          pipeline_change_effect       (gpointer self);

G_END_DECLS


#endif /* __PIPELINE_PHOTO_H__ */

#ifndef _CHEESE_EFFECT_H_
#define _CHEESE_EFFECT_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define CHEESE_TYPE_EFFECT cheese_effect_get_type()

#define CHEESE_EFFECT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHEESE_TYPE_EFFECT, CheeseEffect))

#define CHEESE_EFFECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CHEESE_TYPE_EFFECT, CheeseEffectClass))

#define CHEESE_IS_EFFECT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHEESE_TYPE_EFFECT))

#define CHEESE_IS_EFFECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CHEESE_TYPE_EFFECT))

#define CHEESE_EFFECT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHEESE_TYPE_EFFECT, CheeseEffectClass))

typedef struct {
  GObject parent;
} CheeseEffect;

typedef struct {
  GObjectClass parent_class;
} CheeseEffectClass;

GType cheese_effect_get_type (void);

CheeseEffect* cheese_effect_new (void);

G_END_DECLS

#endif /* _CHEESE_EFFECT_H_ */


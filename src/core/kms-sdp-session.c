#include "kms-sdp-session.h"
#include "kms-sdp-media.h"

#define KMS_SDP_SESSION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), KMS_TYPE_SDP_SESSION, KmsSdpSessionPriv))

#define LOCK(obj) (g_static_mutex_lock(&(KMS_SDP_SESSION(obj)->priv->mutex)))
#define UNLOCK(obj) (g_static_mutex_unlock(&(KMS_SDP_SESSION(obj)->priv->mutex)))

struct _KmsSdpSessionPriv {
	GStaticMutex mutex;
	gchar *addr;
	GValueArray *medias;
	/*
	sdpVersion
	username
	sessionId
	version
	sessionName
	remoteHandler
	 */
};

enum {
	PROP_0,

	PROP_ADDR,
	PROP_MEDIAS
};

G_DEFINE_TYPE(KmsSdpSession, kms_sdp_session, G_TYPE_OBJECT)

static void
free_addr(KmsSdpSession *self) {
	if (self->priv->addr != NULL) {
		g_free(self->priv->addr);
		self->priv->addr = NULL;
	}
}

static void
free_medias(KmsSdpSession *self) {
	if (self->priv->medias != NULL) {
		g_value_array_free(self->priv->medias);
		self->priv->medias = NULL;
	}
}

static void
kms_sdp_session_set_property(GObject  *object, guint property_id,
				const GValue *value, GParamSpec *pspec) {
	KmsSdpSession *self = KMS_SDP_SESSION(object);

	switch (property_id) {
		case PROP_0:
			/* Do nothing */
			break;
		case PROP_ADDR:
			LOCK(self);
			free_addr(self);
			self->priv->addr = g_value_dup_string(value);
			UNLOCK(self);
			break;
		case PROP_MEDIAS:{
			GValueArray *va = g_value_get_boxed(value);
			gint i;

			LOCK(self);
			free_medias(self);
			if (va != NULL)
				self->priv->medias = g_value_array_copy(va);

			for (i=0; i < va->n_values; i++) {
				GValue *v;
				KmsSdpMedia *media;

				v = g_value_array_get_nth(va, i);
				media = g_value_get_object(v);
				g_object_set(media, "session", self, NULL);
				g_object_unref(media);
				g_value_unset(v);
			}

			UNLOCK(self);
			break;
		}
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
	}
}

static void
kms_sdp_session_get_property(GObject *object, guint property_id, GValue *value,
							GParamSpec *pspec) {
	KmsSdpSession *self = KMS_SDP_SESSION(object);

	switch (property_id) {
		case PROP_0:
			break;
		case PROP_ADDR:
			LOCK(self);
			g_value_set_string(value, self->priv->addr);
			UNLOCK(self);
			break;
		case PROP_MEDIAS:
			LOCK(self);
			g_value_set_boxed(value, self->priv->medias);
			UNLOCK(self);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
	}
}

void
kms_sdp_session_dispose(GObject *object) {
	LOCK(object);
	free_medias(KMS_SDP_SESSION(object));
	UNLOCK(object);
	/* Chain up to the parent class */
	G_OBJECT_CLASS (kms_sdp_session_parent_class)->dispose(object);
}

void
kms_sdp_session_finalize(GObject *object) {
	KmsSdpSession *self = KMS_SDP_SESSION(object);

	free_addr(self);
	g_static_mutex_free(&(self->priv->mutex));
	/* Chain up to the parent class */
	G_OBJECT_CLASS (kms_sdp_session_parent_class)->finalize(object);
}

static void
kms_sdp_session_class_init(KmsSdpSessionClass *klass) {
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GParamSpec *pspec, *media;

	g_type_class_add_private(klass, sizeof (KmsSdpSessionPriv));

	gobject_class->set_property = kms_sdp_session_set_property;
	gobject_class->get_property = kms_sdp_session_get_property;
	gobject_class->dispose = kms_sdp_session_dispose;
	gobject_class->finalize = kms_sdp_session_finalize;

	pspec = g_param_spec_string("address", "Address",
					"Remote address",
					"",
					G_PARAM_CONSTRUCT_ONLY |
					G_PARAM_READWRITE);

	g_object_class_install_property(gobject_class, PROP_ADDR, pspec);

	media = g_param_spec_object("media", "Media", "A supported media",
					KMS_TYPE_SDP_MEDIA, G_PARAM_READWRITE);

	pspec = g_param_spec_value_array("medias", "Medias",
					"The medias defines in this session",
					media, G_PARAM_CONSTRUCT_ONLY |
					G_PARAM_READWRITE);

	g_object_class_install_property(gobject_class, PROP_MEDIAS, pspec);
}

static void
kms_sdp_session_init(KmsSdpSession *self) {
	self->priv = KMS_SDP_SESSION_GET_PRIVATE (self);

	g_static_mutex_init(&(self->priv->mutex));
	self->priv->addr = NULL;
	self->priv->medias = NULL;
}
/*
 * Nautilus search-cache Search Provider
 *
 * This file is part of search-and-filter integration for Omarchy Linux.
 * Provides lightning-fast file search using search-cache backend.
 *
 * License: MIT
 * Author: Zack <zack@omarchy.dev>
 */

#define G_LOG_DOMAIN "nautilus-search"

#include <config.h>
#include "nautilus-search-engine-searchcache.h"

#include "nautilus-file.h"
#include "nautilus-global-preferences.h"
#include "nautilus-query.h"
#include "nautilus-search-hit.h"
#include "nautilus-search-provider.h"

#include <string.h>
#include <gio/gio.h>

struct _NautilusSearchEngineSearchCache
{
    GObject parent_instance;

    NautilusQuery *query;
    GCancellable *cancellable;
    GSubprocess *subprocess;  /* Keep subprocess alive until async operation completes */

    gboolean query_pending;
    GQueue *hits_pending;
    guint lazy_timeout_id;  /* Timeout source for lazy hit delivery */
};

static void nautilus_search_provider_init (NautilusSearchProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (NautilusSearchEngineSearchCache,
                         nautilus_search_engine_searchcache,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (NAUTILUS_TYPE_SEARCH_PROVIDER,
                                                nautilus_search_provider_init))

#define BATCH_SIZE 50
#define LAZY_BATCH_SIZE 50
#define MAX_RESULTS 500

static void
finalize (GObject *object)
{
    NautilusSearchEngineSearchCache *self = NAUTILUS_SEARCH_ENGINE_SEARCHCACHE (object);

    /* Cancel pending timeout FIRST to prevent use-after-free */
    if (self->lazy_timeout_id != 0)
    {
        g_source_remove (self->lazy_timeout_id);
        self->lazy_timeout_id = 0;
    }

    if (self->cancellable)
    {
        g_cancellable_cancel (self->cancellable);
        g_clear_object (&self->cancellable);
    }

    g_clear_object (&self->subprocess);
    g_clear_object (&self->query);
    g_queue_free_full (self->hits_pending, g_object_unref);

    G_OBJECT_CLASS (nautilus_search_engine_searchcache_parent_class)->finalize (object);
}

static gboolean
lazy_send_hits_timeout (gpointer user_data)
{
    NautilusSearchEngineSearchCache *self = user_data;

    if (g_queue_is_empty (self->hits_pending))
    {
        self->lazy_timeout_id = 0;  /* Clear ID when source removes itself */
        return G_SOURCE_REMOVE;
    }

    NautilusSearchHit *hit;
    g_autoptr (GPtrArray) hits = g_ptr_array_new_with_free_func (g_object_unref);

    /* Send a small batch to avoid blocking the UI */
    guint count = 0;
    while ((hit = g_queue_pop_head (self->hits_pending)) && count < LAZY_BATCH_SIZE)
    {
        g_ptr_array_add (hits, hit);
        count++;
    }

    if (hits->len > 0)
    {
        nautilus_search_provider_hits_added (NAUTILUS_SEARCH_PROVIDER (self),
                                             g_steal_pointer (&hits));
    }

    /* Continue if more hits pending */
    if (g_queue_is_empty (self->hits_pending))
    {
        self->lazy_timeout_id = 0;  /* Clear ID when source removes itself */
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static void
check_pending_hits (NautilusSearchEngineSearchCache *self,
                    gboolean                         force_send)
{
    if (!force_send &&
        g_queue_get_length (self->hits_pending) < BATCH_SIZE)
    {
        return;
    }

    if (force_send)
    {
        /* Cancel any existing timeout before starting a new one */
        if (self->lazy_timeout_id != 0)
        {
            g_source_remove (self->lazy_timeout_id);
            self->lazy_timeout_id = 0;
        }
        /* Use timeout to lazily send remaining hits with delays to avoid blocking UI */
        self->lazy_timeout_id = g_timeout_add (16, lazy_send_hits_timeout, self);  /* ~60fps */
        return;
    }

    NautilusSearchHit *hit;
    g_autoptr (GPtrArray) hits = g_ptr_array_new_with_free_func (g_object_unref);

    g_debug ("SearchCache engine add hits");

    while ((hit = g_queue_pop_head (self->hits_pending)))
    {
        g_ptr_array_add (hits, hit);
    }

    if (hits->len > 0)
    {
        nautilus_search_provider_hits_added (NAUTILUS_SEARCH_PROVIDER (self),
                                             g_steal_pointer (&hits));
    }
}

static void
search_finished (NautilusSearchEngineSearchCache *self,
                 GError                          *error)
{
    g_debug ("SearchCache engine finished");

    if (error == NULL)
    {
        check_pending_hits (self, TRUE);
    }
    else
    {
        g_queue_foreach (self->hits_pending, (GFunc) g_object_unref, NULL);
        g_queue_clear (self->hits_pending);
    }

    self->query_pending = FALSE;

    if (error && !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
        g_warning ("SearchCache engine error: %s", error->message);
    }
    else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
        g_debug ("SearchCache engine cancelled");
    }

    nautilus_search_provider_finished (NAUTILUS_SEARCH_PROVIDER (self));
}

static void
process_search_results (NautilusSearchEngineSearchCache *self,
                       const gchar                     *output)
{
    g_auto (GStrv) lines = g_strsplit (output, "\n", -1);

    for (gsize i = 0; lines[i] != NULL; i++)
    {
        const gchar *line = g_strstrip (lines[i]);

        if (line[0] == '\0')
        {
            continue;
        }

        // Skip "No files found" message from sc command
        if (g_str_has_prefix (line, "No files found"))
        {
            continue;
        }

        // Only process lines that look like absolute paths
        if (line[0] != '/')
        {
            continue;
        }

        // Create GFile from path and get URI
        g_autoptr (GFile) file = g_file_new_for_path (line);
        g_autofree gchar *uri = g_file_get_uri (file);
        g_autofree gchar *basename = g_file_get_basename (file);

        // Create search hit with URI
        NautilusSearchHit *hit = nautilus_search_hit_new (uri);

        // Compute match score based on filename (prefix matches rank higher)
        gdouble match = nautilus_query_matches_string (self->query, basename);
        if (match < 0)
        {
            match = 0.5;  // Fallback if no match computed
        }
        nautilus_search_hit_set_fts_rank (hit, match);

        // Add to pending hits
        g_queue_push_tail (self->hits_pending, hit);

        // Batch send hits
        check_pending_hits (self, FALSE);
    }
}

static void
search_callback (GObject      *source_object,
                GAsyncResult *result,
                gpointer      user_data)
{
    NautilusSearchEngineSearchCache *self = user_data;
    g_autoptr (GError) error = NULL;

    g_autofree gchar *stdout_str = NULL;
    g_autofree gchar *stderr_str = NULL;
    gint exit_status;

    GSubprocess *subprocess = G_SUBPROCESS (source_object);

    if (!g_subprocess_communicate_utf8_finish (subprocess,
                                               result,
                                               &stdout_str,
                                               &stderr_str,
                                               &error))
    {
        // FALLBACK: On communication error, silently finish (let other engines work)
        g_debug ("SearchCache engine: communication failed, falling back silently");
        g_clear_object (&self->subprocess);  /* Reap the subprocess */
        search_finished (self, NULL);
        return;
    }

    exit_status = g_subprocess_get_exit_status (subprocess);

    if (exit_status != 0)
    {
        // FALLBACK: On sc error (missing index, bad query), silently finish
        // Don't report error to user - other search engines (simple, tracker) will provide results
        g_debug ("SearchCache engine: sc exited with status %d, falling back: %s",
                 exit_status, stderr_str ? stderr_str : "");
        g_clear_object (&self->subprocess);  /* Reap the subprocess */
        search_finished (self, NULL);
        return;
    }

    if (stdout_str && stdout_str[0] != '\0')
    {
        process_search_results (self, stdout_str);
    }

    g_clear_object (&self->subprocess);  /* Reap the subprocess */
    search_finished (self, NULL);
}

static gboolean
search_engine_searchcache_start (NautilusSearchProvider *provider,
                                 NautilusQuery          *query)
{
    NautilusSearchEngineSearchCache *self = NAUTILUS_SEARCH_ENGINE_SEARCHCACHE (provider);

    g_debug ("SearchCache engine start");

    if (self->query_pending)
    {
        g_debug ("SearchCache engine already running, cancelling");
        g_cancellable_cancel (self->cancellable);
        g_clear_object (&self->cancellable);
        g_clear_object (&self->subprocess);  /* Clean up old subprocess */
    }

    g_clear_object (&self->query);
    self->query = g_object_ref (query);
    self->cancellable = g_cancellable_new ();

    g_autofree gchar *search_text = nautilus_query_get_text (query);

    if (search_text == NULL || search_text[0] == '\0')
    {
        g_debug ("SearchCache engine: empty query");
        return FALSE;
    }

    g_debug ("SearchCache engine: searching for '%s'", search_text);

    // RESILIENCE: Check if sc binary exists in PATH
    // If not, silently return and let other search engines handle it
    g_autofree gchar *sc_path = g_find_program_in_path ("sc");
    if (sc_path == NULL)
    {
        g_debug ("SearchCache engine: 'sc' not found in PATH, skipping");
        nautilus_search_provider_finished (NAUTILUS_SEARCH_PROVIDER (self));
        return FALSE;
    }

    // Build command: sc --full-path <query>
    g_autoptr (GError) error = NULL;
    g_autoptr (GSubprocessLauncher) launcher = g_subprocess_launcher_new (
        G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE);

    // Launch subprocess with result limit - use query limit if set, otherwise use preferences
    guint query_limit = nautilus_query_get_max_results (self->query);
    guint result_limit = (query_limit > 0) ? query_limit :
                         g_settings_get_uint (nautilus_preferences, NAUTILUS_PREFERENCES_SEARCH_RESULTS_LIMIT);
    g_autofree gchar *limit_str = g_strdup_printf ("%u", result_limit);
    GSubprocess *subprocess;

    // Always search globally - proximity bonus in relevance scoring will rank
    // results closer to the search location higher
    // Use daemon mode for better performance (scd keeps index in memory)
    subprocess = g_subprocess_launcher_spawn (launcher, &error,
                                              "sc",
                                              "--full-path",
                                              "--limit", limit_str,
                                              search_text,
                                              NULL);

    if (subprocess == NULL)
    {
        g_warning ("Failed to spawn search-cache: %s", error->message);
        search_finished (self, error);
        return FALSE;
    }

    self->query_pending = TRUE;

    // Store subprocess reference to prevent zombies and ensure proper cleanup
    self->subprocess = subprocess;

    // Communicate asynchronously
    g_subprocess_communicate_utf8_async (subprocess,
                                         NULL,
                                         self->cancellable,
                                         search_callback,
                                         self);

    return TRUE;
}

static void
nautilus_search_engine_searchcache_stop (NautilusSearchProvider *provider)
{
    NautilusSearchEngineSearchCache *self = NAUTILUS_SEARCH_ENGINE_SEARCHCACHE (provider);

    g_debug ("SearchCache engine stop");

    /* Cancel pending timeout to prevent use-after-free */
    if (self->lazy_timeout_id != 0)
    {
        g_source_remove (self->lazy_timeout_id);
        self->lazy_timeout_id = 0;
    }

    if (self->query_pending)
    {
        g_cancellable_cancel (self->cancellable);
        g_clear_object (&self->cancellable);
        self->query_pending = FALSE;
    }

    /* Clean up pending hits that won't be delivered */
    g_queue_foreach (self->hits_pending, (GFunc) g_object_unref, NULL);
    g_queue_clear (self->hits_pending);

    /* Clean up subprocess to prevent zombies */
    g_clear_object (&self->subprocess);
}

static void
nautilus_search_provider_init (NautilusSearchProviderInterface *iface)
{
    iface->start = search_engine_searchcache_start;
    iface->stop = nautilus_search_engine_searchcache_stop;
}

static void
nautilus_search_engine_searchcache_class_init (NautilusSearchEngineSearchCacheClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);

    object_class->finalize = finalize;
}

static void
nautilus_search_engine_searchcache_init (NautilusSearchEngineSearchCache *self)
{
    self->hits_pending = g_queue_new ();

    g_debug ("SearchCache provider initialized");
}

NautilusSearchEngineSearchCache *
nautilus_search_engine_searchcache_new (void)
{
    return g_object_new (NAUTILUS_TYPE_SEARCH_ENGINE_SEARCHCACHE, NULL);
}

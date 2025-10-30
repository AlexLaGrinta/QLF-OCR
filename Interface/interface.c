#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    GtkWidget *entry_rotate;
    GtkWidget *image_preview;
    char image_path[512];
    GtkWidget *window;
    GdkPixbuf *pixbuf_original;
    GdkPixbuf *pixbuf_display;
} AppWidgets;

static void on_open_clicked(GtkWidget *widget, gpointer data);
static void on_rotate_clicked(GtkWidget *widget, gpointer data);
static void on_ocr_clicked(GtkWidget *widget, gpointer data);
static void on_preproc_clicked(GtkWidget *widget, gpointer data);
static void apply_dark_theme(GtkWidget *window);
static void on_fullscreen_toggle(GtkWidget *widget, gpointer data);
static GdkPixbuf* rotate_pixbuf_bilinear_fixed(const GdkPixbuf *src, double angle_deg);

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Crosswords Resolver");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    apply_dark_theme(window);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_top, FALSE, FALSE, 0);

    GtkWidget *btn_open = gtk_button_new_with_label("Ouvrir une image PNG");
    GtkWidget *btn_ocr = gtk_button_new_with_label("OCR");
    GtkWidget *btn_preproc = gtk_button_new_with_label("Preprocessing");
    GtkWidget *btn_fullscreen = gtk_button_new_with_label("Plein écran");
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_open, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_ocr, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_preproc, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox_top), btn_fullscreen, FALSE, FALSE, 0);

    GtkWidget *hbox_rotate = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *entry_rotate = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_rotate), "Degrés (ex: 90)");
    GtkWidget *btn_rotate = gtk_button_new_with_label("Rotation");
    gtk_box_pack_start(GTK_BOX(hbox_rotate), entry_rotate, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_rotate), btn_rotate, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_rotate, FALSE, FALSE, 0);

    GtkWidget *image_preview = gtk_image_new();
    gtk_widget_set_halign(image_preview, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(image_preview, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), image_preview, TRUE, TRUE, 0);

    AppWidgets *app = g_malloc(sizeof(AppWidgets));
    app->entry_rotate = entry_rotate;
    app->image_preview = image_preview;
    app->window = window;
    app->pixbuf_original = NULL;
    app->pixbuf_display = NULL;
    app->image_path[0] = '\0';

    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_clicked), app);
    g_signal_connect(btn_rotate, "clicked", G_CALLBACK(on_rotate_clicked), app);
    g_signal_connect(btn_ocr, "clicked", G_CALLBACK(on_ocr_clicked), app);
    g_signal_connect(btn_preproc, "clicked", G_CALLBACK(on_preproc_clicked), app);
    g_signal_connect(btn_fullscreen, "clicked", G_CALLBACK(on_fullscreen_toggle), app);

    gtk_widget_show_all(window);
    gtk_main();

    if (app->pixbuf_original) g_object_unref(app->pixbuf_original);
    if (app->pixbuf_display) g_object_unref(app->pixbuf_display);
    g_free(app);
    return 0;
}

static void apply_dark_theme(GtkWidget *window) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window, button, entry, box {background-color:#1e1e1e;color:#f0f0f0;font-family:'Sans';font-size:14px;}"
        "button {background:#333;border-radius:8px;padding:6px 10px;}button:hover {background:#555;}"
        "entry {background:#2b2b2b;border-radius:6px;color:#fff;border:1px solid #444;padding:4px;}",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
}

static void on_open_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Choisir une image PNG", GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN, "_Annuler", GTK_RESPONSE_CANCEL, "_Ouvrir", GTK_RESPONSE_ACCEPT, NULL);
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        strncpy(app->image_path, file, sizeof(app->image_path)-1);
        app->image_path[sizeof(app->image_path)-1] = '\0';
        g_free(file);
        if (app->pixbuf_original) g_object_unref(app->pixbuf_original);
        app->pixbuf_original = gdk_pixbuf_new_from_file(app->image_path, NULL);
        if (app->pixbuf_display) g_object_unref(app->pixbuf_display);
        app->pixbuf_display = gdk_pixbuf_copy(app->pixbuf_original);
        gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_preview), app->pixbuf_display);
    }
    gtk_widget_destroy(dialog);
}

static GdkPixbuf* rotate_pixbuf_bilinear_fixed(const GdkPixbuf *src, double angle_deg) {
    int w = gdk_pixbuf_get_width(src), h = gdk_pixbuf_get_height(src);
    int rowstride = gdk_pixbuf_get_rowstride(src), n = gdk_pixbuf_get_n_channels(src);
    guchar *sp = gdk_pixbuf_get_pixels(src);
    GdkPixbuf *dst = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
    guchar *dp = gdk_pixbuf_get_pixels(dst);
    int drow = gdk_pixbuf_get_rowstride(dst);
    double a = angle_deg * M_PI / 180.0, ca = cos(a), sa = sin(a);
    double cx = (w - 1) / 2.0, cy = (h - 1) / 2.0;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            double rx = (x - cx) * ca + (y - cy) * sa + cx;
            double ry = -(x - cx) * sa + (y - cy) * ca + cy;
            int xi = floor(rx), yi = floor(ry);
            double dx = rx - xi, dy = ry - yi;
            guchar *d = dp + y * drow + x * 4;
            if (xi >= 0 && xi + 1 < w && yi >= 0 && yi + 1 < h) {
                guchar *p1 = sp + yi * rowstride + xi * n;
                guchar *p2 = sp + yi * rowstride + (xi + 1) * n;
                guchar *p3 = sp + (yi + 1) * rowstride + xi * n;
                guchar *p4 = sp + (yi + 1) * rowstride + (xi + 1) * n;
                for (int c = 0; c < 3; c++) {
                    double v = (1 - dx) * (1 - dy) * p1[c] +
                               dx * (1 - dy) * p2[c] +
                               (1 - dx) * dy * p3[c] +
                               dx * dy * p4[c];
                    d[c] = (guchar)v;
                }
                d[3] = 255;
            } else {
                d[0] = d[1] = d[2] = 255; d[3] = 255;
            }
        }
    }
    return dst;
}

static void on_rotate_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = data;
    if (!app->pixbuf_original) return;
    const gchar *deg = gtk_entry_get_text(GTK_ENTRY(app->entry_rotate));
    if (!deg || !*deg) return;
    double a = atof(deg);
    if (app->pixbuf_display) g_object_unref(app->pixbuf_display);
    app->pixbuf_display = rotate_pixbuf_bilinear_fixed(app->pixbuf_original, a);
    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_preview), app->pixbuf_display);
    gdk_pixbuf_save(app->pixbuf_display, "output.bmp", "bmp", NULL, NULL);
}

static void on_ocr_clicked(GtkWidget *widget, gpointer data) { g_print("OCR...\n"); }
static void on_preproc_clicked(GtkWidget *widget, gpointer data) { g_print("Preproc...\n"); }

static void on_fullscreen_toggle(GtkWidget *widget, gpointer data) {
    AppWidgets *app = data;
    static gboolean full = FALSE;
    if (!full) {
        gtk_window_fullscreen(GTK_WINDOW(app->window));
        gtk_button_set_label(GTK_BUTTON(widget), "Quitter plein écran");
    } else {
        gtk_window_unfullscreen(GTK_WINDOW(app->window));
        gtk_button_set_label(GTK_BUTTON(widget), "Plein écran");
    }
    full = !full;
}


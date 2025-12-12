#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "../RotationPNG/rotate_png.h"
#include "../Preprocessing/preprocessing.h"
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
static void on_auto_rotate_clicked(GtkWidget *widget, gpointer data);
static void on_preproc_clicked(GtkWidget *widget, gpointer data);
static void on_save_clicked(GtkWidget *widget, gpointer data);
static void on_ocr_clicked(GtkWidget *widget, gpointer data);
static void apply_dark_theme(GtkWidget *window);
static void on_fullscreen_toggle(GtkWidget *widget, gpointer data);
int automatic_rotate(const char* input_path, const char* output_path);
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Searchwords Resolver");
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
    GtkWidget *btn_auto_rotate = gtk_button_new_with_label("Rotation Auto");
    GtkWidget *btn_save = gtk_button_new_with_label("Sauvegarder");
    GtkWidget *btn_fullscreen = gtk_button_new_with_label("Plein écran");

    gtk_box_pack_start(GTK_BOX(hbox_top), btn_open, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_ocr, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_preproc, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_auto_rotate, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_save, FALSE, FALSE, 0);
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
    g_signal_connect(btn_auto_rotate, "clicked", G_CALLBACK(on_auto_rotate_clicked), app);
    g_signal_connect(btn_preproc, "clicked", G_CALLBACK(on_preproc_clicked), app);
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_clicked), app);
    g_signal_connect(btn_ocr, "clicked", G_CALLBACK(on_ocr_clicked), app);
    g_signal_connect(btn_fullscreen, "clicked", G_CALLBACK(on_fullscreen_toggle), app);

    gtk_widget_show_all(window);
    gtk_main();

    if (app->pixbuf_original) g_object_unref(app->pixbuf_original);
    if (app->pixbuf_display) g_object_unref(app->pixbuf_display);
    g_free(app);
    return 0;
}

static void on_fullscreen_toggle(GtkWidget *widget, gpointer data) {
    AppWidgets *app = data;
    static gboolean full = FALSE;

    if (!app->pixbuf_display) return;

    if (!full) {
        gtk_window_fullscreen(GTK_WINDOW(app->window));
        gtk_button_set_label(GTK_BUTTON(widget), "Quitter plein écran");
    } else {
        gtk_window_unfullscreen(GTK_WINDOW(app->window));
        gtk_button_set_label(GTK_BUTTON(widget), "Plein écran");
    }

    full = !full;

    int win_width, win_height;
    gtk_window_get_size(GTK_WINDOW(app->window), &win_width, &win_height);

    if (app->pixbuf_display && app->pixbuf_original) {
        g_object_unref(app->pixbuf_display);
        GdkPixbuf *scaled = gdk_pixbuf_scale_simple(app->pixbuf_original,
            win_width - 40, win_height - 80, GDK_INTERP_BILINEAR);
        app->pixbuf_display = scaled;
        gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_preview), app->pixbuf_display);
    }
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
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Choisir une image", GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN, "_Annuler", GTK_RESPONSE_CANCEL, "_Ouvrir", GTK_RESPONSE_ACCEPT, NULL);

    GtkFileFilter *filter_all = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_all, "Images");
    gtk_file_filter_add_pattern(filter_all, "*.png");
    gtk_file_filter_add_pattern(filter_all, "*.jpg");
    gtk_file_filter_add_pattern(filter_all, "*.jpeg");
    gtk_file_filter_add_pattern(filter_all, "*.bmp");
    gtk_file_filter_add_pattern(filter_all, "*.gif");
    gtk_file_filter_add_pattern(filter_all, "*.tiff");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_all);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        strncpy(app->image_path, file, sizeof(app->image_path)-1);
        app->image_path[sizeof(app->image_path)-1] = '\0';
        g_free(file);

        if (app->pixbuf_original) g_object_unref(app->pixbuf_original);
        app->pixbuf_original = gdk_pixbuf_new_from_file(app->image_path, NULL);
        if (!app->pixbuf_original) {
            g_print("Erreur: impossible de charger l'image.\n");
            gtk_widget_destroy(dialog);
            return;
        }

        if (app->pixbuf_display) g_object_unref(app->pixbuf_display);
        app->pixbuf_display = gdk_pixbuf_copy(app->pixbuf_original);
        gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_preview), app->pixbuf_display);
    }
    gtk_widget_destroy(dialog);
}

static void on_rotate_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = data;
    if (!app->pixbuf_original) return;

    const gchar *deg = gtk_entry_get_text(GTK_ENTRY(app->entry_rotate));
    if (!deg || !*deg) return;

    double angle = atof(deg);

    if (app->pixbuf_display) g_object_unref(app->pixbuf_display);

    // Utilisation de la fonction externe dans rotate_png.c
    app->pixbuf_display = rotate_pixbuf_bilinear_fixed(app->pixbuf_original, angle);

    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_preview), app->pixbuf_display);
    g_print("Image tournée de %.2f degrés.\n", angle);
}


static void on_preproc_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = data;
    if (!app->pixbuf_original || strlen(app->image_path) == 0) return;

    const char* outName = "output.bmp";
    int res = preprocess_image(app->image_path, outName);
    if (res != 0) {
        g_print("Erreur lors du preprocessing.\n");
        return;
    }

    char outPath[512];
    snprintf(outPath, sizeof(outPath), "temp/%s", outName);

    if (app->pixbuf_display) g_object_unref(app->pixbuf_display);
    app->pixbuf_display = gdk_pixbuf_new_from_file(outPath, NULL);
    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_preview), app->pixbuf_display);
}

static void on_save_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = data;
    if (!app->pixbuf_display) return;

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Enregistrer l'image", GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_SAVE, "_Annuler", GTK_RESPONSE_CANCEL, "_Enregistrer", GTK_RESPONSE_ACCEPT, NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.bmp");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        const char *ext = strrchr(filename, '.');
        const char *type = "bmp";
        if (ext) {
            if (strcasecmp(ext, ".png") == 0) type = "png";
            else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) type = "jpg";
        }

        GError *err = NULL;
        if (!gdk_pixbuf_save(app->pixbuf_display, filename, type, &err, NULL)) {
            g_print("Erreur lors de la sauvegarde : %s\n", err->message);
            g_error_free(err);
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

static void on_ocr_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = data;
    if (!app->pixbuf_original || strlen(app->image_path) == 0) return;

    const char* auto_rotated_path = "auto_rotated.bmp";

    // Rotation automatique
    if (automatic_rotate(app->image_path, auto_rotated_path) != 0) {
        g_print("Erreur lors de la rotation automatique.\n");
        return;
    }

    // Préprocessing sur l'image déjà rotatée
    const char* preproc_name = "preproc_output.bmp";
    if (preprocess_image(auto_rotated_path, preproc_name) != 0) {
        g_print("Erreur lors du preprocessing.\n");
        return;
    }

    // Affichage du résultat pré-traité
    char preproc_path[512];
    snprintf(preproc_path, sizeof(preproc_path), "temp/%s", preproc_name);

    if (app->pixbuf_display) g_object_unref(app->pixbuf_display);
    app->pixbuf_display = gdk_pixbuf_new_from_file(preproc_path, NULL);
    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_preview), app->pixbuf_display);

    g_print("OCR prêt à être exécuté sur l'image prétraitée.\n");
    // Ici tu peux ajouter l'appel réel à ta fonction OCR si tu en as une
}
static void on_auto_rotate_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = data;
    if (!app->pixbuf_original || strlen(app->image_path) == 0) return;

    const char* output_path = "auto_rotated.bmp";

    // Appel à la fonction dans ../RotationPNG/rotate_png.c
    if (automatic_rotate(app->image_path, output_path) == 0) {
        if (app->pixbuf_display) g_object_unref(app->pixbuf_display);
        app->pixbuf_display = gdk_pixbuf_new_from_file(output_path, NULL);
        gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_preview), app->pixbuf_display);
        g_print("Rotation automatique terminée.\n");
    } else {
        g_print("Erreur lors de la rotation automatique.\n");
    }
}


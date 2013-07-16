#ifndef WINDOW_H
#define WINDOW_H

#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QToolBar>
#include <QMainWindow>
#include <QStyle>
#include <QPrinter>
#include <QPrintPreviewWidget>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>

class window : public QMainWindow
{
	Q_OBJECT

	private:
		QToolBar toolbar;
		QPrinter printer_settings;
		QPrintPreviewWidget/*QPrintPreviewDialog*/ printer;
		QImage input;

		QHBoxLayout splitter;
		QVBoxLayout settings;

		QWidget helper;

		QLabel image;
		QLabel text_oben;
		QDoubleSpinBox edit_oben;
		QLabel text_unten;
		QDoubleSpinBox edit_unten;
		QLabel text_h;
		QDoubleSpinBox edit_h;

		QWidget dummy;

		double tasse_oben;
		double tasse_unten;
		double tasse_h;
		double tasse_U_oben;
		double tasse_U_unten;
		double tasse_alpha;
		double tasse_r_oben;
		double tasse_r_unten;

		std::pair<double, double> rect2cup(std::pair<double, double> pos);
		double rect2cupMM();
		double rect2cupMiddleMM();
		double rect2cupHeightMM();

	public:
		window(QWidget *parent = 0);
		~window();

	private:
		QImage renderImage(float pixel_per_mm_x=11.8, float pixel_per_mm_y=11.8);

	private slots:
		void open();
		void save();
		void print();
		void printPreview(QPrinter *printer);
		void updateTasse();
};

#endif // WINDOW_H

#include "window.h"
#include <QString>
#include <QFileDialog>
#include <QMessageBox>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <cmath>
#include <QDebug>
#include <QColor>
#include <QProgressDialog>
#include <QIcon>
#include <QImageReader>

namespace tassen_bild
{
	#include "image.h"
}
window::window(QWidget *parent): QMainWindow(parent), printer_settings(QPrinter::ScreenResolution)
{
	toolbar.addAction(QIcon(QApplication::applicationDirPath()+"/icons/document-open-symbolic.png"), "Öffnen", this,  SLOT(open()));
	toolbar.addAction(QIcon(QApplication::applicationDirPath()+"/icons/document-save-as-symbolic.png"), "Speichern als ...", this,  SLOT(save()));
	toolbar.addSeparator();
	toolbar.addAction(QIcon(QApplication::applicationDirPath()+"/icons/printer-symbolic.png"), "Drucken", this, SLOT(print()));

	toolbar.setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

	addToolBar(&toolbar);

	printer_settings.setFullPage(true);
	printer_settings.setOrientation(QPrinter::Landscape);

	//load image
	unsigned char *pixel = new unsigned char[tassen_bild::width*tassen_bild::height*3];
	unsigned char *pixel_ptr = pixel;
	char *image_ptr = tassen_bild::header_data;
	for(unsigned i = 0; i < tassen_bild::width*tassen_bild::height; ++i)
	{
		HEADER_PIXEL(image_ptr, pixel_ptr)
		pixel_ptr += 3;
	}

	//image.setPixmap(QPixmap::fromImage(QImage(pixel, tassen_bild::width, tassen_bild::height, QImage::Format_RGB888)));
	image.setPixmap(QPixmap::fromImage(QImage(QApplication::applicationDirPath()+"/icons/tasse.png")));
	settings.addWidget(&image);

	text_oben.setText("Durchmesser oben:");
	settings.addWidget(&text_oben);
	edit_oben.setValue(90.0);
	edit_oben.setMaximum(1000.0);
	edit_oben.setAlignment(Qt::AlignTop);
	connect(&edit_oben, SIGNAL(valueChanged(double)), SLOT(updateTasse()));
	settings.addWidget(&edit_oben);

	text_unten.setText("Durchmesser unten:");
	settings.addWidget(&text_unten);
	edit_unten.setValue(60.0);
	edit_unten.setMaximum(1000.0);
	connect(&edit_unten, SIGNAL(valueChanged(double)), SLOT(updateTasse()));
	settings.addWidget(&edit_unten);

	text_h.setText("Seitenhöhe:");
	settings.addWidget(&text_h);
	edit_h.setMaximum(1000.0);
	edit_h.setValue(100.0);
	connect(&edit_h, SIGNAL(valueChanged(double)), SLOT(updateTasse()));
	settings.addWidget(&edit_h);

	settings.addWidget(&dummy, 10000);

	delete[] pixel;

	splitter.addLayout(&settings);
	splitter.addWidget(&printer);
	helper.setLayout(&splitter);
	setCentralWidget(&helper);

	setMinimumWidth(800);
	setMinimumHeight(400);

	//updateTasse();
}

void window::open()
{
	QString filter = "Image Files (";

	for (int i  = 0; i < QImageReader::supportedImageFormats().size(); ++i)
	{
		filter += "*.";
		filter += QImageReader::supportedImageFormats().at(i).data();
		filter += " ";
	}
	filter += ")";
	filter += " ;; All Files (*.*)";

	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath(), filter);
	if (!fileName.isEmpty())
	{
		QImage image(fileName);
		if (image.isNull()) {
			QMessageBox::information(this, tr("koku-cup-printer"), tr("Cannot load %1.").arg(fileName));
			return;
		}
		input = image;

		updateTasse();
	}
}

void window::save()
{
	if (input.isNull())
	{
		QMessageBox::information(this, tr("koku-cup-printer"), tr("Öffne eine Datei zuerst !"));
		return;
	}
	QString filter = "Image Files (";

	for (int i  = 0; i < QImageReader::supportedImageFormats().size(); ++i)
	{
		filter += "*.";
		filter += QImageReader::supportedImageFormats().at(i).data();
		filter += " ";
	}
	filter += ")";
	filter += " ;; All Files (*.*)";

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::currentPath(), filter);
	if (!fileName.isEmpty())
	{
		//do something
		QPrinter printer(QPrinter::HighResolution); //PrinterMode { ScreenResolution, PrinterResolution, HighResolution };
		printer.setFullPage(true);
		printer.setOrientation(QPrinter::Landscape);
		renderImage(printer.width()/printer.widthMM(), printer.height()/printer.heightMM()).save(fileName);
	}
}

void window::print()
{
	QPrinter printer(QPrinter::HighResolution); //PrinterMode { ScreenResolution, PrinterResolution, HighResolution };
	printer.setFullPage(true);
	printer.setOrientation(QPrinter::Landscape);

	QPrintDialog *dialog = new QPrintDialog(&printer, this);
	dialog->setWindowTitle(tr("Print Document"));

	if (dialog->exec() != QDialog::Accepted)
	{
		return;
	}

	printPreview(&printer);
}

//Calculates RECT-Coordiantes to CUP-Coordinates
std::pair<double, double> window::rect2cup(std::pair<double, double> pos)
{
	//remove dimensions, 1 == tasse_h
	//double t_oben    = tasse_oben/tasse_r_oben;
	//double t_unten   = tasse_unten/tasse_r_oben;
	//double t_h       = tasse_h/tasse_r_oben;
	//double t_U_oben  = tasse_U_oben/tasse_r_oben;
	//double t_U_unten = tasse_U_unten/tasse_r_oben;
	double t_alpha   = tasse_alpha;
	double t_r_oben  = tasse_r_oben/tasse_r_oben;
	double t_r_unten = tasse_r_unten/tasse_r_oben;

	double dx = pos.first;
	double dy = pos.second;

	double r = sqrt(dx*dx+dy*dy);

	double real_y = (r-t_r_unten)/(t_r_oben-t_r_unten);

	double real_x = (t_alpha/2.0-atan(dx/dy))/t_alpha;

	//double ang = acos(dy/r);
	//double real_x = (t_alpha/2.0-ang)/t_alpha;

	return std::make_pair(real_x, real_y);
}

double window::rect2cupMM()
{
	return tasse_r_oben;
}

double window::rect2cupMiddleMM()
{
	return (tasse_U_oben+tasse_U_unten)/2.0;
}

double window::rect2cupHeightMM()
{
	return tasse_h;
}
#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif
void window::updateTasse()
{
	tasse_oben = edit_oben.value();
	tasse_unten = edit_unten.value();
	tasse_h = edit_h.value();
	//update values
	tasse_U_oben  = tasse_oben*M_PI;
	tasse_U_unten = tasse_unten*M_PI;

	double a = tasse_h;
	double b = (tasse_oben-tasse_unten)/2.0;

	tasse_alpha   = asin(b/a);
	qDebug() << "alpha: " << tasse_alpha/(2*M_PI)*360 << endl;
	tasse_r_oben  = (tasse_oben/2.0)/sin(tasse_alpha);
	tasse_r_unten = (tasse_unten/2.0)/sin(tasse_alpha);

	tasse_alpha = tasse_U_oben/tasse_r_oben;

	qDebug() << "r oben: " << tasse_r_oben << endl;
	qDebug() << "r unten: " << tasse_r_unten << endl;
	qDebug() << "U oben: " << tasse_U_oben << endl;
	qDebug() << "U unten: " << tasse_U_unten << endl;
	qDebug() << "U oben recalc: " << tasse_r_oben*tasse_alpha << endl;
	qDebug() << "U unten recalc: " << tasse_r_unten*tasse_alpha << endl;
	//rerender

	//reset printer
	/*(&printer)->~QPrintPreviewDialog();
	new (&printer) QPrintPreviewDialog(&printer_settings);*/
	printer.setLandscapeOrientation();
	connect(&printer, SIGNAL(paintRequested(QPrinter *)), SLOT(printPreview(QPrinter *)));
	printer.show();
	printer.updatePreview();
	splitter.addWidget(&printer);
}

QImage window::renderImage(float pixel_per_mm_x, float pixel_per_mm_y)
{
	if (!input.isNull())
	{
		int check_w = 2.0*rect2cupMM()*pixel_per_mm_x;
		int check_h = rect2cupMM()*pixel_per_mm_y;

		QProgressDialog progress("Generating image...", "Abort", 0, check_h, this);
		progress.setWindowModality(Qt::WindowModal);
	//this check is stupid:
		int x_low = check_w;
		int x_high = 0;
		int y_low = check_h;
		int y_high = 0;
		for(int y = 0; y < check_h; y+=std::max(check_h/1000,1))
		{
			if (progress.wasCanceled())
			{
				break;
			}
			progress.setValue(progress.value()+std::max(check_h/1000,1));
			for(int x = 0; x < check_w; x+=std::max(check_w/1000,1))
			{
				double rx = (x / double(check_w) - 0.5)*2;
				double ry = (y / double(check_w) - 0.5)*2;

				std::pair<double, double> c = rect2cup(std::make_pair(rx, ry));

				if (c.first < 0) continue;
				if (c.second < 0) continue;
				if (c.first >= 1) continue;
				if (c.second >= 1) continue;

				x_low = std::min(x_low, x);
				y_low = std::min(y_low, y);

				x_high = std::max(x_high, x);
				y_high = std::max(y_high, y);
			}
		}
	//generate image
		x_low -= 3*std::max(check_h/1000,10);
		y_low -= 3*std::max(check_h/1000,10);
		x_high += 3*std::max(check_h/1000,10);
		y_high += 3*std::max(check_h/1000,10);

		qDebug() << "X: " << x_low << " - " << x_high;
		qDebug() << "Y: " << y_low << " - " << y_high;

		int ow = x_high-x_low;
		int oh = y_high-y_low;

		double border_mm = 5.0/rect2cupMM();
		double border_mm2 = 7.0/rect2cupMM();

		progress.setValue(0);
		progress.setRange(0, oh);

		QImage output(ow, oh, /*QImage::Format_RGB888*/QImage::Format_ARGB32);
		output.setDotsPerMeterX(pixel_per_mm_x*1000);
		output.setDotsPerMeterY(pixel_per_mm_y*1000);
		for(int y = y_low; y < y_high+1; ++y)
		{
			if (progress.wasCanceled())
			{
				break;
			}
			progress.setValue(progress.value()+1);
			for(int x = x_low; x < x_high+1; ++x)
			{
				double rx = (x / double(check_w) - 0.5)*2;
				double ry = (y / double(check_w) - 0.5)*2;

				int ox = x-x_low;
				int oy = y-y_low;

				if (ox < 0) continue;
				if (oy < 0) continue;
				if (ox >= ow) continue;
				if (oy >= oh) continue;

				output.setPixel(ox, oy, QColor(255, 255, 255, 0).rgba());

				//get data
				std::pair<double, double> c = rect2cup(std::make_pair(rx, ry));

				//border
				if ((c.first < 0-border_mm) || (c.second < 0-border_mm) || (c.first >= 1+border_mm) || (c.second >= 1+border_mm))
				{
					output.setPixel(ox, oy, QColor(128, 128, 128, 128).rgba());
				}

				if ((c.first < 0-border_mm2) || (c.second < 0-border_mm2) || (c.first >= 1+border_mm2) || (c.second >= 1+border_mm2))
				{
					output.setPixel(ox, oy, QColor(255, 255, 255, 0).rgba());
				}

				//c.first = 0-1 => 0-w aber ! size unabhängig von h
				c.first = 0.5 + (c.first - 0.5)*rect2cupMiddleMM()/rect2cupHeightMM()*input.height()/input.width();

				int ix = c.first*input.width();
				int iy = c.second*input.height();

				iy = input.height()-1 - iy; //flip

				if (ix < 0) continue;
				if (iy < 0) continue;
				if (ix >= input.width()) continue;
				if (iy >= input.height()) continue;

				output.setPixel(ox, oy, input.pixel(ix, iy));
			}
		}

		//painter.drawImage(painter.device()->width()/2-ow/2, painter.device()->height()/2-oh/2, output);
		return output;
	}
	return QImage();
}

void window::printPreview(QPrinter *printer)
{
	QPainter painter(printer);

	if (!input.isNull())
	{
		QImage output = renderImage(painter.device()->width()/painter.device()->widthMM(), painter.device()->height()/painter.device()->heightMM());
		painter.drawImage(painter.device()->width()/2-output.width()/2, painter.device()->height()/2-output.height()/2, output);
	}
}

window::~window()
{
	//I hate QTs memory managment
	exit(0); //clean exit
}

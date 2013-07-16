#include <QApplication>
#include "window.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	window my_window;
	my_window.show();

	return a.exec();
}

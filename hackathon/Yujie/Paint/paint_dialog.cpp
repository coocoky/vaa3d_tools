#include "paint_dialog.h"
#include "scribblearea.h"

Paint_Dialog::Paint_Dialog(V3DPluginCallback2 *cb, QWidget *parent) :
    QDialog(parent)
{
    callback=cb;
    paintarea = new ScribbleArea();
    image1Dc_in=0;
    create();
    previousz=-1;
    int dataflag=0; //if loaded data,dataflag=0; if fetched,dataflag=1
}

void Paint_Dialog::create()
{

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(paintarea,1,0,1,1);
    QToolBar *tool = new QToolBar;
    tool->setGeometry(0,0,250,20);
    //tool->setOrientation(Qt::Vertical );

    QVBoxLayout *layout = new QVBoxLayout;
    QToolButton *button_load = new QToolButton;
    button_load->setGeometry(0,0,10,20);
    button_load->setText("Load");
    QToolButton *button_save = new QToolButton;
    button_save->setText("Save");
    button_save->setGeometry(0,0,10,20);
    QToolButton *button_color = new QToolButton;
    button_color->setText("Color");
    QToolButton *button_fetch = new QToolButton;
    button_fetch->setText("Fetch");
    QToolButton *button_pb = new QToolButton;
    button_pb->setText("Pushback");
    button_pb->setGeometry(0,0,10,20);
    QToolButton *button_pen = new QToolButton;
    button_pen->setText("Pen Width");
    QToolButton *button_print = new QToolButton;
    button_print->setText("Print");
    QToolButton *button_clear = new QToolButton;
    button_clear->setText("Clear screen");
    button_clear->setGeometry(0,0,10,20);
    QToolButton *button_zoomin=new QToolButton;
    button_zoomin->setText("Zoom in");
    button_zoomin->setGeometry(0,0,10,20);
    QToolButton *button_zoomout=new QToolButton;
    button_zoomout->setText("Zoom out");
    button_zoomout->setGeometry(0,0,10,20);

    QLabel *label= new QLabel;
    label->setText("Information of selecton:");
    edit=new QPlainTextEdit;
    edit->setPlainText("");
    spin=new QSpinBox;
    spin->setMaximum(0);
    spin->setMinimum(1);
    connect(spin,SIGNAL(valueChanged(int)),this,SLOT(zdisplay(int)));
    connect(button_pb,SIGNAL(clicked()),this,SLOT(pushback()));

    tool->addWidget(button_load);
    tool->addSeparator();
    tool->addWidget(button_fetch);
    tool->addSeparator();
    tool->addWidget(button_save);
    tool->addSeparator();
    tool->addWidget(button_pb);
    tool->addSeparator();
    tool->addWidget(button_zoomin);
    tool->addSeparator();
    tool->addWidget(button_zoomout);
    tool->addSeparator();
    tool->addWidget(button_color);
    tool->addSeparator();
    tool->addWidget(button_pen);
    tool->addSeparator();
    tool->addWidget(button_clear);
    tool->addSeparator();
    tool->addWidget(button_print);
    tool->addSeparator();
    tool->addWidget(spin);

    layout->addWidget(tool);
    gridLayout->addWidget(label,2,0,1,1);
    gridLayout->addLayout(layout,0,0,1,1);
    gridLayout->addWidget(edit,3,0,1,1);

    this->setLayout(gridLayout);
    this->setMinimumHeight(800);
    this->setMinimumWidth(500);
    connect(button_load, SIGNAL(clicked()), this, SLOT(load()));
    connect(button_save, SIGNAL(clicked()), this, SLOT(save()));
    connect(button_color, SIGNAL(clicked()), this, SLOT(penColor()));
    connect(button_pen, SIGNAL(clicked()), this, SLOT(penWidth()));
    connect(button_clear, SIGNAL(clicked()), this, SLOT(clearimage()));
    connect(button_print, SIGNAL(clicked()), paintarea, SLOT(print()));
    connect(button_fetch, SIGNAL(clicked()), this, SLOT(fetch()));
    connect(button_zoomin,SIGNAL(clicked()),this, SLOT(zoomin()));
    connect(button_zoomout,SIGNAL(clicked()),this,SLOT(zoomout()));
}


bool Paint_Dialog::maybeSave()
{
    if (paintarea->isModified()) {
       QMessageBox::StandardButton ret;
       ret = QMessageBox::warning(this, tr("Paint"),
                          tr("The image has been modified.\n"
                             "Do you want to save your changes?"),
                          QMessageBox::Save | QMessageBox::Discard
                          | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            return saveFile("jpg");
        } else if (ret == QMessageBox::Cancel) {
            return false;
        }
        qDebug()<<"MaybeSave";
    }
    return true;
}


bool Paint_Dialog::load()
{
    qDebug()<<"In load now";
    if (maybeSave())
    {fileName = QFileDialog::getOpenFileName(0, QObject::tr("Choose the input image "),
                                         QDir::currentPath(),
       QObject::tr("Images (*.raw *.tif *.lsm *.v3dpbd *.v3draw);;All(*)"));
    }
    if (!fileName.isEmpty())
    {
        qDebug()<<"1";

        if (!simple_loadimage_wrapper(*callback, fileName.toStdString().c_str(), image1Dc_in, sz_img, intype))
        {
            v3d_msg("load image "+fileName+" error!");
            return false;
        }
        qDebug()<<"2";

        disdata=datacopy(image1Dc_in,sz_img[0]*sz_img[1]*sz_img[2]*sz_img[3]);
        qDebug()<<"3";

        QSize newSize;
        newSize.setWidth(sz_img[0]);
        newSize.setHeight(sz_img[1]);
        paintarea->setFixedSize(newSize);
        qDebug()<<"4";
        spin->setMaximum(sz_img[2]-1);
        spin->setValue(sz_img[2]/2);
        //spin change value will trigger zdisplay
        return true;
    }
    return false;
}

void Paint_Dialog::zdisplay(int z)
{
    if (previousz>=0)
    {
      savezimage(previousz);
    }

    QSize newSize;
    newSize.setWidth(sz_img[0]);
    newSize.setHeight(sz_img[1]);
    QImage newimage(newSize, QImage::Format_RGB16);
    QRgb value;

    z=z-2;
    if (z<0)  z=0; //bug in the v3d main

    int p2=0;
    int p3=0;
    //Write image data to display on the screen
    for(int x=0; x< sz_img[0]; x++){
        for(int y=0; y<sz_img[1]; y++){
            int p1=disdata[x+sz_img[0]*y+z*sz_img[0]*sz_img[1]];
            if (sz_img[3]>1)
            {
                p2=disdata[x+sz_img[0]*y+z*sz_img[0]*sz_img[1]+sz_img[0]*sz_img[1]*sz_img[2]];
            }
            if (sz_img[3]>2)
            {
                p3=disdata[x+sz_img[0]*y+z*sz_img[0]*sz_img[1]+2*sz_img[0]*sz_img[1]*sz_img[2]];
            }
            value=qRgb(p1,p2,p3);
            newimage.setPixel(x,y,value);
        }
    }
    qDebug()<<"The end of for loop in zdisplay";

    //newimage.scaled(500,800,Qt::KeepAspectRatio);

    //paintarea->openImage(newimage.scaled(500,800,Qt::KeepAspectRatio));
    paintarea->openImage(newimage);

    QString tmp="Image Size: \nx: " + QString::number(sz_img[0]) + " y: " + QString::number(sz_img[1]) +
            " z: " + QString::number(sz_img[2]) + "\nCurrent z: " + QString::number(spin->value());

    edit->setPlainText(tmp);

    previousz=spin->value();
}


void Paint_Dialog::clearimage()
{
    previousz=-1;
    if (dataflag==1)
    { unsigned char * tmp=disdata;
      disdata=data1d;
     zdisplay(spin->value());
      disdata=tmp;
    }
    else
    {
      unsigned char * tmp1=disdata;
      disdata=image1Dc_in;
      zdisplay(spin->value());
      disdata=tmp1;
    }

}


void Paint_Dialog::fetch()
{
    qDebug()<<"In fetch now";
    curwin = callback->currentImageWindow();
    if (!curwin)
    {
        QMessageBox::information(0, "", "You don't have any image open in the main window.");
        return;
    }

    Image4DSimple* p4DImage = callback->getImage(curwin);

    if (!p4DImage)
    {
        QMessageBox::information(0, "", "The image pointer is invalid. Ensure your data is valid and try again!");
        return;
    }


    data1d = p4DImage->getRawData();
    ImagePixelType pixeltype = p4DImage->getDatatype();

    V3DLONG N = p4DImage->getXDim();
    sz_img[0]=N;
    V3DLONG M = p4DImage->getYDim();
    sz_img[1]=M;
    V3DLONG P = p4DImage->getZDim();
    sz_img[2]=P;
    V3DLONG sc = p4DImage->getCDim();
    sz_img[3]=sc;

    dataflag=1;

    //save image1Dc_in data in qcopydata
    disdata=datacopy(data1d,sz_img[0]*sz_img[1]*sz_img[2]*sz_img[3]);

    QSize newSize;
    newSize.setWidth(sz_img[0]);
    newSize.setHeight(sz_img[1]);
    paintarea->setFixedSize(newSize);

    TriviewControl *tript=callback->getTriviewControl(curwin);
    V3DLONG x,y,z;
    tript->getFocusLocation(x,y,z);
    qDebug()<<"fetch z value:"<<z;
    spin->setMaximum(sz_img[2]);
    spin->setValue(z);

}


unsigned char * Paint_Dialog::datacopy(unsigned char *data,long size)
{
    unsigned char * qcopydata=new unsigned char [size];
    for (int i=0;i<size;i++)
    {
        qcopydata[i]=data[i];
    }
    qDebug()<<"I have been copied";
    return qcopydata;
}


bool Paint_Dialog::zoomin()
{
    QSize newSize;
    newSize.setWidth(500);
    newSize.setHeight(800);
    paintarea->setFixedSize(newSize);
    //imagecopy=paintarea->image;
    paintarea->openImage(paintarea->image.scaled(500,800,Qt::KeepAspectRatio));
    return true;
}

void Paint_Dialog::zoomout()
{
    //paintarea->image=imagecopy;
    paintarea->openImage(paintarea->image.scaled(sz_img[0],sz_img[1],Qt::KeepAspectRatio));
}

void Paint_Dialog::savezimage(int z)
{
    if (!paintarea->isModified())
    {
        qDebug()<<"I have not changed";
        return;
    }
    else
    {
        QColor color;

        z=z-2;
        if (z<0) z=0;

        //accomodate data with less than 3 channels.

        for(int x=0; x< sz_img[0]; x++){
            for(int y=0; y<sz_img[1]; y++){
                color=paintarea->image.pixel(QPoint(x,y));
                int red=color.red();
                int blue=color.blue();
                int green=color.green();
                disdata[x+sz_img[0]*y+z*sz_img[0]*sz_img[1]]=red;
                if (sz_img[3]>1)
                {
                disdata[x+sz_img[0]*y+z*sz_img[0]*sz_img[1]+sz_img[2]*sz_img[0]*sz_img[1]]=green;
                }
                if (sz_img[3]>2)
                {
                disdata[x+sz_img[0]*y+z*sz_img[0]*sz_img[1]+2*sz_img[2]*sz_img[0]*sz_img[1]]=blue;
                }
             }
        }
     qDebug()<<"z image is saved: "<<z;
    }
}

void Paint_Dialog::pushback()
{   qDebug()<<"Inpushback now";

    curwin = callback->currentImageWindow();
    if (!curwin)
    {
        QMessageBox::information(0, "", "You don't have any image open in the main window.");
        return;
    }

    if (zoomin())
       zoomout();
    QColor color;
    int z=spin->value();
    savezimage(z);

    for(int i=0; i<sz_img[0]*sz_img[1]*sz_img[2]*sz_img[3]; i++){
        data1d[i]=disdata[i];
    }

    Image4DSimple * new4DImage = new Image4DSimple();
    callback->setImage(curwin, new4DImage);
    callback->setImageName(curwin, "Paint result");
    callback->updateImageWindow(curwin);
    qDebug()<<"After updatewindow";

}

bool Paint_Dialog::saveFile(const QByteArray &fileFormat)
{
    QString initialPath = QDir::currentPath() + "/untitled." + fileFormat;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                               initialPath,
                               tr("%1 Files (*.%2);;All Files (*)")
                               .arg(QString(fileFormat.toUpper()))
                               .arg(QString(fileFormat)));
    if (fileName.isEmpty()) {
        return false;
    } else {
        return paintarea->saveImage(fileName, fileFormat);
    }
    qDebug()<<"SaveFile";
}


void Paint_Dialog::save()
{

    saveFile("jpg");


}

void Paint_Dialog::penColor()
{
    QColor newColor = QColorDialog::getColor(paintarea->penColor());
    if (newColor.isValid())
        paintarea->setPenColor(newColor);
}

void Paint_Dialog::penWidth()
{
    bool ok;
    int newWidth = QInputDialog::getInteger(this, tr("Paint"),
                                            tr("Select pen width:"),
                                            paintarea->penWidth(),
                                            1, 50, 1, &ok);
    if (ok)
        paintarea->setPenWidth(newWidth);
}
#include "tilecache.h"
#include <QDir>
#include <QStandardPaths>
#include <QMutexLocker>
#include <QDebug>
#include <QApplication>
TileCache::TileCache()
{
	QDir dir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    dir.mkpath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/GOOGLE");
    dir.mkpath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/OSM");

    m_cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/OSM";
    qDebug()<< Q_FUNC_INFO<< m_cacheLocation;

    UserAgent = QByteArray("Chrome/51.0.2704.103 Safari/537.36");

	start();

}

void TileCache::SetGoogle()
{
    qDebug()<< Q_FUNC_INFO;
    m_tileTypes = GOOGLE_TILES;
    m_cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/GOOGLE";
    m_tileExistsMap.clear();
    zoomLevelChanged();
}

void TileCache::setOSM()
{
    qDebug()<< Q_FUNC_INFO;
    m_tileTypes = OSM_TILES;
    m_cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/OSM";
    m_tileExistsMap.clear();
    zoomLevelChanged();
}



int TileCache::Random(int low, int high)
{
    return low + std::rand() % (high - low);
}

void TileCache::zoomLevelChanged()
{
	for (int i=0;i<m_tileIdList.keys().size();i++)
	{
		m_tileIdList.keys().at(i)->abort();
	}

	m_tileIdList.clear();

	emit networkTileUpdate(0);
}

void TileCache::getTile(int x, int y, int z)
{
	QMutexLocker locker(&m_tileMutex);
	TileReq req;
	req.x = x;
	req.y = y;
	req.z = z;
	m_tileQueue.append(req);
	return;
	QDir cachedir(m_cacheLocation);
	if (!cachedir.exists(QString::number(x)))
	{
		//return QImage();
	}
	cachedir.cd(QString::number(x));
	if (!cachedir.exists(QString::number(y)))
	{
		//return QImage();
	}
	cachedir.cd(QString::number(y));
	if (cachedir.exists(QString::number(z) + ".png"))
	{
		//cachedir.remove(QString::number(z) + ".png");
		QImage img;
		img.load(cachedir.absoluteFilePath(QString::number(z) + ".png"));
		//return img;
	}
	//return QImage();
	//return m_tileMap.value(x).value(y).value(z);
}
void TileCache::add(int x, int y, int z, QImage img)
{
	QDir cachedir(m_cacheLocation);
	if (!cachedir.exists(QString::number(x)))
	{
		cachedir.mkdir(QString::number(x));
	}
	if (!m_tileExistsMap.contains(x))
	{
		m_tileExistsMap[x] = QMap<int,QMap<int,bool> >();
	}
	cachedir.cd(QString::number(x));
	if (!cachedir.exists(QString::number(y)))
	{
		cachedir.mkdir(QString::number(y));
	}
	if (!m_tileExistsMap[x].contains(y))
	{
		m_tileExistsMap[x][y] = QMap<int,bool>();
	}
	cachedir.cd(QString::number(y));
	if (cachedir.exists(QString::number(z) + ".png"))
	{
		//cachedir.remove(QString::number(z) + ".png");
	}
	if (!m_tileExistsMap[x][y].contains(z))
	{
		m_tileExistsMap[x][y][z] = true;
	}
	//QFile imgfile(cachedir.absoluteFilePath(QString::number(z) + ".png"));
	//imgfile.open(QIODevice::ReadWrite | QIODevice::Truncate);
	img.save(cachedir.absoluteFilePath(QString::number(z) + ".png"));
	return;
	/*if (!m_tileMap.contains(x))
	{
		m_tileMap[x] = QMap<int,QMap<int,QImage> >();
	}
	if (!m_tileMap.value(x).contains(y))
	{
		m_tileMap[x][y] = QMap<int,QImage>();
	}
	m_tileMap[x][y][z] = img;*/
}

bool TileCache::contains(int x, int y, int z)
{
	if (m_tileExistsMap.contains(x))
	{
		if (m_tileExistsMap.value(x).contains(y))
		{
			if (m_tileExistsMap.value(x).value(y).contains(z))
			{
				return m_tileExistsMap.value(x).value(y).value(z);
			}
		}
	}
	return false;
	QDir cachedir(m_cacheLocation);
	if (cachedir.exists(QString::number(x)))
	{
		cachedir.cd(QString::number(x));
		if (cachedir.exists(QString::number(y)))
		{
			cachedir.cd(QString::number(y));
			if (cachedir.exists(QString::number(z) + ".png"))
			{
				return true;
			}
		}
	}
	return false;
	/*if (m_tileMap.contains(x))
	{
		if (m_tileMap.value(x).contains(y))
		{
			if (m_tileMap.value(x).value(y).contains(zoom))
			{
				return true;
			}
		}
	}
	return false;*/
}
void TileCache::run()
{
	m_nam = new QNetworkAccessManager();
	qDebug() << QSslSocket::sslLibraryBuildVersionString();
	qDebug() << QSslSocket::sslLibraryVersionString();

	//Populate initial cache list
	QDir cachedir(m_cacheLocation);
	QStringList xlist = cachedir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	foreach (QString x,xlist)
	{
		if (!m_tileExistsMap.contains(x.toInt()))
		{
			m_tileExistsMap[x.toInt()] = QMap<int,QMap<int,bool> >();
		}
		cachedir.cd(x);
		QStringList ylist = cachedir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
		foreach (QString y, ylist)
		{
			if (!m_tileExistsMap[x.toInt()].contains(y.toInt()))
			{
				m_tileExistsMap[x.toInt()][y.toInt()] = QMap<int,bool>();
			}
			cachedir.cd(y);
			QStringList zlist = cachedir.entryList(QDir::Files | QDir::NoDotAndDotDot);
			foreach (QString z,zlist)
			{
				if (z.contains("."))
				{
					QString zstr = z.split(".")[0];
					if (!m_tileExistsMap[x.toInt()][y.toInt()].contains(zstr.toInt()))
					{
						m_tileExistsMap[x.toInt()][y.toInt()][zstr.toInt()] = true;
					}
				}
			}
			cachedir.cdUp();
		}
		cachedir.cdUp();
	}
	qDebug() << "Finished loading current tiles";

	QList<TileReq> privReqList;
	while (true)
	{
		m_tileMutex.lock();
		for (int i=0;i<m_tileQueue.size();i++)
		{
			privReqList.append(m_tileQueue.at(i));
		}
		m_tileQueue.clear();
		m_tileMutex.unlock();
		for (int i=0;i<privReqList.size();i++)
		{
			bool found = true;
			QDir cachedir(m_cacheLocation);
			if (cachedir.exists(QString::number(privReqList.at(i).x)))
			{
				cachedir.cd(QString::number(privReqList.at(i).x));
				if (cachedir.exists(QString::number(privReqList.at(i).y)))
				{
					cachedir.cd(QString::number(privReqList.at(i).y));
					if (cachedir.exists(QString::number(privReqList.at(i).z) + ".png"))
					{
						//cachedir.remove(QString::number(z) + ".png");
						QImage img;
						img.load(cachedir.absoluteFilePath(QString::number(privReqList.at(i).z) + ".png"));
						//return img;
						emit tileRecv(privReqList.at(i).x,privReqList.at(i).y,privReqList.at(i).z,img);
						emit localTileUpdate(privReqList.size() - (i+1));
						continue;
					}
				}
			}
			//No image found here, queue it up to be grabbed from the network.
            QNetworkRequest req;

            QString url;

            if (m_tileTypes == GOOGLE_TILES)
            {
                // Google:
                url = "https://mt0.google.com/vt/";
                QString loc = "lyrs=s&x=%1&y=%2&z=%3";
                req.setRawHeader("User-Agent",UserAgent);


                req.setUrl(url + loc.arg(privReqList.at(i).x).arg(privReqList.at(i).y).arg(privReqList.at(i).z));


                url = req.url().toString();
            }

            else if (m_tileTypes == OSM_TILES)
            {
                url = ("http://tile.openstreetmap.org/${z}/${x}/${y}.png");
                req.setRawHeader("User-Agent",UserAgent);

                url.replace("${z}", QString::number(privReqList.at(i).z));
                url.replace("${x}", QString::number(privReqList.at(i).x));
                url.replace("${y}", QString::number(privReqList.at(i).y));



                req.setUrl(url);


            }

            qDebug()<< Q_FUNC_INFO<< url << m_tileTypes;;


			QNetworkReply *reply = m_nam->get(req);
			connect(reply,SIGNAL(finished()),this,SLOT(networkFinished()));


            connect(reply, &QNetworkReply::errorOccurred,this, &TileCache::networkError);

			emit networkTileUpdate(m_tileIdList.keys().count());
			m_tileIdList[reply] = QPair<QPair<int,int> , int>(QPair<int,int>(privReqList.at(i).x,privReqList.at(i).y),privReqList.at(i).z);
		}
		if (privReqList.size() == 0)
		{
			msleep(10);
		}
		else
		{
			privReqList.clear();
			emit localTileUpdate(0);
		}
		QApplication::processEvents();
	}
}
void TileCache::networkError(QNetworkReply::NetworkError err)
{
    qDebug() << "Network Error:" << err;
}

void TileCache::networkFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	if (reply->error() != QNetworkReply::NoError)
	{
		return;
	}
	//qDebug() << reply->errorString();
	QByteArray imgdata = reply->readAll();
	QImage img;
	if (!img.loadFromData(imgdata))
	{
		qDebug() << "Failed to create IMG from web data";
		qDebug() << QString(imgdata);
		emit networkTileUpdate(m_tileIdList.keys().count()-1);
		m_tileIdList.remove(reply);
		return;
	}
	//qDebug() << "Downloaded tile" << m_tileIdList.keys().count();

	emit networkTileUpdate(m_tileIdList.keys().count()-1);
	emit tileRecv(m_tileIdList[reply].first.first,m_tileIdList[reply].first.second,m_tileIdList[reply].second,img);
	this->add(m_tileIdList[reply].first.first,m_tileIdList[reply].first.second,m_tileIdList[reply].second,img);
	m_tileIdList.remove(reply);
	//m_scene->addItem(t);
}

#include "imageView.h"
#include <QScrollBar>
#include <QGraphicsPixmapItem>
void centerImageInView(QGraphicsView* view, QGraphicsPixmapItem* pixmapItem) {
	if (!view || !pixmapItem) return;

	// 1. 获取图片的边界矩形（场景坐标）
	QRectF itemRect = pixmapItem->boundingRect();

	// 2. 设置场景的可见范围（避免滚动条出现在小图片上）
	view->scene()->setSceneRect(itemRect);

	// 3. 计算视图中心与图片中心的偏移
	QRectF viewRect = view->mapToScene(view->viewport()->rect()).boundingRect();
	QPointF viewCenter = viewRect.center();
	QPointF itemCenter = itemRect.center();

	// 4. 移动图片中心到视图中心
	pixmapItem->setPos(viewCenter - itemCenter);

	// 5. 可选：重置缩放（确保图片完整可见）
	view->fitInView(pixmapItem, Qt::KeepAspectRatio);
}
InteractiveGraphicsView::InteractiveGraphicsView(QWidget* parent)
	: QGraphicsView(parent)
{
	setRenderHint(QPainter::Antialiasing);
	setDragMode(NoDrag); // 禁用默认拖拽模式，手动实现
	setTransformationAnchor(AnchorUnderMouse); // 缩放时以鼠标为中心
	setResizeAnchor(AnchorViewCenter); // 窗口大小变化时保持中心
}

// 设置缩放因子
void InteractiveGraphicsView::setZoomFactor(double factor) {
	factor = qBound(ZOOM_MIN, factor, ZOOM_MAX);
	double scaleChange = factor / m_zoom;
	scale(scaleChange, scaleChange);
	m_zoom = factor;
}

// 鼠标按下：开始拖动
void InteractiveGraphicsView::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		m_dragging = true;
		m_lastMousePos = event->pos();
		setCursor(Qt::ClosedHandCursor);
	}
	QGraphicsView::mousePressEvent(event);
}

// 鼠标移动：拖动视图
void InteractiveGraphicsView::mouseMoveEvent(QMouseEvent* event) {
	if (m_dragging) {
		QPoint delta = event->pos() - m_lastMousePos;
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
		verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
		m_lastMousePos = event->pos();
	}
	QGraphicsView::mouseMoveEvent(event);
}

// 鼠标释放：结束拖动
void InteractiveGraphicsView::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		m_dragging = false;
		setCursor(Qt::ArrowCursor);
	}
	QGraphicsView::mouseReleaseEvent(event);
}

// 鼠标双击：居中当前项
void InteractiveGraphicsView::mouseDoubleClickEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		//centerOnCurrentItem();
		emit sendDoublickSignals();
	}
	QGraphicsView::mouseDoubleClickEvent(event);
}

// 滚轮缩放
void InteractiveGraphicsView::wheelEvent(QWheelEvent* event) {
	double angle = event->angleDelta().y();
	double factor = (angle > 0) ? 1.1 : 0.9; // 放大10%或缩小10%
	scaleView(factor);
}

void InteractiveGraphicsView::keyPressEvent(QKeyEvent* event)
{

	const int step = 20; // 平移步长（像素）
	const double zoomStep = 0.1; // 缩放步长

	switch (event->key()) {
		// 方向键平移
	case Qt::Key_Left:
		moveViewByKeyboard(-step, 0);
		break;
	case Qt::Key_Right:
		moveViewByKeyboard(step, 0);
		break;
	case Qt::Key_Up:
		moveViewByKeyboard(0, -step);
		break;
	case Qt::Key_Down:
		moveViewByKeyboard(0, step);
		break;

		// 快捷键缩放（Ctrl++/Ctrl+-）
	case Qt::Key_Plus:
		if (event->modifiers() & Qt::ControlModifier) {
			zoomByKeyboard(true);
		}
		break;
	case Qt::Key_Minus:
		if (event->modifiers() & Qt::ControlModifier) {
			zoomByKeyboard(false);
		}
		break;

		// 空格键居中
	case Qt::Key_Space:
		centerOnCurrentItem();
		break;

	default:
		QGraphicsView::keyPressEvent(event); // 其他键交给父类处理
	}
}

// 键盘移动视图
void InteractiveGraphicsView::moveViewByKeyboard(int dx, int dy) {
	horizontalScrollBar()->setValue(horizontalScrollBar()->value() + dx);
	verticalScrollBar()->setValue(verticalScrollBar()->value() + dy);
}

// 键盘缩放视图
void InteractiveGraphicsView::zoomByKeyboard(bool zoomIn) {
	double factor = zoomIn ? 1.0 + zoomStep : 1.0 - zoomStep;
	scaleView(factor);
}

// 居中当前项
void InteractiveGraphicsView::centerOnCurrentItem() {
	if (scene() && !scene()->items().isEmpty()) {

		QGraphicsItem* item = scene()->items().first(); // 假设第一个项是图片
		centerImageInView(this, (QGraphicsPixmapItem*)item);

		/*	QRectF itemRect = item->mapRectToScene(item->boundingRect());
			QRectF viewRect = mapToScene(viewport()->rect()).boundingRect();

			 计算居中位置
			QPointF targetPos = viewRect.center() - itemRect.center();
			item->setPos(targetPos);*/
	}
}

// 执行缩放（保持鼠标位置固定）
void InteractiveGraphicsView::scaleView(qreal factor) {
	double newZoom = m_zoom * factor;
	if (newZoom < ZOOM_MIN || newZoom > ZOOM_MAX) return;

	// 保存当前鼠标场景坐标
	QPointF oldScenePos = mapToScene(m_lastMousePos);

	// 应用缩放
	scale(factor, factor);
	m_zoom = newZoom;

	// 调整滚动条保持鼠标位置不变
	QPointF newViewPos = mapFromScene(oldScenePos);
	QPointF delta = newViewPos - m_lastMousePos;
	horizontalScrollBar()->setValue(horizontalScrollBar()->value() + delta.x());
	verticalScrollBar()->setValue(verticalScrollBar()->value() + delta.y());
}

viewWidget::viewWidget(QWidget* parent) : QWidget(parent)
{
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint
		| Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);


	init();
	connect(view_ptr.get(), &InteractiveGraphicsView::sendDoublickSignals, this, [=]() {emit sendDoublickSignals(m_key); });
}

viewWidget::viewWidget(viewWidget* copy)
{
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint
		| Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
	init();

	mainitem->setPixmap(copy->getImage());

	int width = copy->getImage().width();
	int height = copy->getImage().height();
	view_ptr->scene()->addItem(mainitem);
	view_ptr->centerOn(mainitem);
	view_ptr->fitInView(mainitem, Qt::KeepAspectRatio);
	labptr->setText(copy->getName());
	NGCounts = copy->NGCounts;
	defcetMap = copy->defcetMap;
}

viewWidget::~viewWidget()
{
	if (mainitem)
	{
		delete mainitem;
		mainitem = nullptr;
	}
	qDebug() << __FUNCTION__ << " line:" << __LINE__ << " m_key:" << m_key << " delete success!";
}

void viewWidget::init()
{
	setAttribute(Qt::WA_Hover, true);
	installEventFilter(this);
	setMouseTracking(true); // 启用鼠标跟踪
	QString  m_style = "QLabel{background-color:rgb(211,211,211);color:rgb(50,50,50);border-top-left-radius:8px;border-top-right-radius:8px}";
	// 图像
	mainitem = new QGraphicsPixmapItem;
	mainitem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemContainsChildrenInShape);
	QLabel* m_lab = new QLabel;
	m_lab->setStyleSheet(m_style);
	m_lab->setFixedHeight(20);

	labptr.reset(m_lab);

	// 显示窗口
	InteractiveGraphicsView* view = new InteractiveGraphicsView;
	view->setScene(&scene);

	//view->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	view_ptr.reset(view);
	QVBoxLayout* vBox = new QVBoxLayout;
	layout_ptr.reset(vBox);
	vBox->addWidget(m_lab);
	vBox->addWidget(view);
	vBox->setSpacing(0);

	vBox->setContentsMargins(0, 0, 0, 0);
	delete this->layout();
	this->setLayout(vBox);
	//centerImageInView(view_ptr.get(), mainitem);


}

void viewWidget::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);
	if (mainitem)
		centerImageInView(view_ptr.get(), mainitem);
}

void viewWidget::showNGInfo()
{
	QString showtxt;
	showtxt.append(tr("NG次数:") + QString::number(NGCounts) + "\n");
	QStringList defectList = defcetMap.keys();
	QStringList defecttxt;
	for (auto& defect : defectList)
	{
		defecttxt << QString(defect).append(":" + QString::number(defcetMap.value(defect)));
	}
	showtxt.append(defecttxt.join("\n"));
	QToolTip::showText(QCursor::pos(), showtxt, this, this->rect(), 5000);
}

void viewWidget::setName(QString name)
{
	m_Name = name;
	labptr->setText(tr("  检测区域: ") + name);
}

void viewWidget::reciveImage(QString key, QImage& image)
{
	try {
		foreach(QGraphicsItem * item, view_ptr->scene()->items())
		{
			view_ptr->scene()->removeItem(item);
		}
		QPixmap   map;
		if (!image.isNull())
		{
			if (!map.convertFromImage(image))
			{
				qCritical() << __FUNCTION__ << " line:" << __LINE__ << "convertFromImage failed";
				return;
			}
		}
		else
		{
			qCritical() << __FUNCTION__ << " line:" << __LINE__ << "image is Null";
			return;
		}
		mainitem->setPixmap(map);
		view_ptr->scene()->addItem(mainitem);
		centerImageInView(view_ptr.get(), mainitem);
		//view_ptr->scene()->update();

	}
	catch (QString ev) {
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << " ev:" << ev;
	}
}
void viewWidget::clearData()
{
	defcetMap.clear();
	NGCounts = 0;
}
QPixmap viewWidget::getImage()
{
	return  mainitem->pixmap();
}
void viewWidget::reciveResult(QString result, QByteArray defectArray)
{
	if (result.contains("NG"))
	{
		NGCounts++;
	}
	QJsonArray array = QJsonDocument::fromJson(defectArray).array();
	QStringList keys = defcetMap.keys();
	for (auto& defectstr : array)
	{
		QString defect = defectstr.toString().split("@")[0];
		if (keys.contains(defect))
		{
			defcetMap.insert(defect, defcetMap.value(defect) + 1);
		}
		else
		{
			defcetMap.insert(defect, 1);
		}
	}
}
//void viewWidget::mousePressEvent(QMouseEvent* event)
//{
//    if (event->button() == Qt::LeftButton)
//    {
//        QPoint point = event->pos();
//        if (point.y() <= 32)
//        {
//        std:: cout << "ddadadad";
//        }
//    }
//
//}

//------------------------------------------------------------------------------
//  particleenvelopewidget.h
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "particleenvelopewidget.h"
#include "math/scalar.h"

using namespace Math;
namespace Particles
{

//------------------------------------------------------------------------------
/**
*/
ParticleEnvelopeWidget::ParticleEnvelopeWidget(QWidget* parent) :
	QFrame(parent),
	activePoint(0)
{
	// setup ui
	this->ui.setupUi(this);

	this->ui.xVal->setSingleStep(0.01f);
	this->ui.yVal->setSingleStep(0.01f);
	this->ui.min->setSingleStep(0.01f);
	this->ui.max->setSingleStep(0.01f);
	this->ui.frequency->setSingleStep(0.01f);
	this->ui.amplitude->setSingleStep(0.01f);

	connect(this->ui.xVal, SIGNAL(valueChanged(double)), this, SLOT(PointValueChanged()));
	connect(this->ui.yVal, SIGNAL(valueChanged(double)), this, SLOT(PointValueChanged()));

	connect(this->ui.renderWidget, SIGNAL(PointUpdated(QPointF)), this, SLOT(PointValueUpdated(QPointF)));
	connect(this->ui.renderWidget, SIGNAL(PointSelected(int)), this, SLOT(PointSelected(int)));

	connect(this->ui.frequency, SIGNAL(valueChanged(double)), this, SLOT(FrequencyChanged(double)));
	connect(this->ui.amplitude, SIGNAL(valueChanged(double)), this, SLOT(AmplitudeChanged(double)));
	connect(this->ui.function, SIGNAL(currentIndexChanged(int)), this, SLOT(FunctionChanged(int)));

	connect(this->ui.min, SIGNAL(valueChanged(double)), this, SLOT(MinValueChanged(double)));
	connect(this->ui.max, SIGNAL(valueChanged(double)), this, SLOT(MaxValueChanged(double)));
}

//------------------------------------------------------------------------------
/**
*/
ParticleEnvelopeWidget::~ParticleEnvelopeWidget()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleEnvelopeWidget::Setup( const EnvelopeCurve& curve, EmitterAttrs::EnvelopeAttr attr )
{
	// block all signals
	this->ui.xVal->blockSignals(true);
	this->ui.yVal->blockSignals(true);
	this->ui.min->blockSignals(true);
	this->ui.max->blockSignals(true);
	this->ui.frequency->blockSignals(true);
	this->ui.amplitude->blockSignals(true);
	this->ui.function->blockSignals(true);
	this->ui.renderWidget->blockSignals(true);

	// set curve and attr
	this->curve = curve;
	this->attr = attr;

	// get values
	const float* values = curve.GetValues();
	const float* limits = curve.GetLimits();

	// update ui
	this->ui.min->setValue(limits[0]);
	this->ui.min->setMaximum(limits[1]);
	this->ui.max->setValue(limits[1]);
	this->ui.max->setMinimum(limits[0]);
	this->ui.frequency->setValue(curve.GetFrequency());
	this->ui.amplitude->setValue(curve.GetAmplitude());
	this->ui.function->setCurrentIndex(curve.GetModFunc());

	// calculate span
	float span = limits[1] - limits[0];

	// calculate points
	QPointF p0(0.0f, (values[0]-limits[0])/span);
	QPointF p1(curve.GetKeyPos0(), (values[1]-limits[0])/span);
	QPointF p2(curve.GetKeyPos1(), (values[2]-limits[0])/span);
	QPointF p3(1.0f, (values[3]-limits[0])/span);

	// setup from points
	this->ui.renderWidget->Setup(p0, p1, p2, p3);

	// unlock signals
	this->ui.xVal->blockSignals(false);
	this->ui.yVal->blockSignals(false);
	this->ui.min->blockSignals(false);
	this->ui.max->blockSignals(false);
	this->ui.frequency->blockSignals(false);
	this->ui.amplitude->blockSignals(false);
	this->ui.function->blockSignals(false);
	this->ui.renderWidget->blockSignals(false);
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleEnvelopeWidget::PointValueChanged()
{
	// get values
	float values[4] = { this->curve.GetValues()[0], this->curve.GetValues()[1], this->curve.GetValues()[2], this->curve.GetValues()[3] };

	// get max values
	const float* limits = this->curve.GetLimits();

	// calculate span
	float span = limits[1] - limits[0];

	// calculate y-val
	float yVal = this->ui.yVal->value() / span - limits[0];
	float xVal = n_clamp(this->ui.xVal->value(), 0.0f, 1.0f);

	switch(this->activePoint)
	{
	case 0:
		values[this->activePoint] = yVal;
		break;
	case 1:
		this->curve.SetKeyPos0(xVal);
		values[this->activePoint] = yVal;
		break;
	case 2:
		this->curve.SetKeyPos1(xVal);
		values[this->activePoint] = yVal;
		break;
	case 3:
		values[this->activePoint] = yVal;
		break;
	}

	// set values
	this->curve.SetValues(values[0], values[1], values[2], values[3]);

	// call widget
	this->ui.renderWidget->SelectedPointValueChanged(QPointF(xVal, yVal));

	// emit signal
	emit this->ValueChanged(this->attr);
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleEnvelopeWidget::PointValueUpdated(QPointF point)
{
	// get limits
	const float* limits = this->curve.GetLimits();

	// calculate span
	float span = limits[1] - limits[0];

	// calculate y-val
	float yVal = point.y() * span + limits[0];
	float xVal = n_clamp(point.x(), 0.0f, 1.0f);

	// get values
	float values[4] = { this->curve.GetValues()[0], this->curve.GetValues()[1], this->curve.GetValues()[2], this->curve.GetValues()[3] };

	// disable signals
	this->ui.xVal->blockSignals(true);
	this->ui.yVal->blockSignals(true);

	switch(this->activePoint)
	{
	case 0:
		this->ui.yVal->setValue(yVal);
		values[this->activePoint] = yVal;
		break;
	case 1:
		this->curve.SetKeyPos0(xVal);
		this->ui.xVal->setValue(xVal);
		this->ui.yVal->setValue(yVal);
		values[this->activePoint] = yVal;
		break;
	case 2:
		this->curve.SetKeyPos1(xVal);
		this->ui.xVal->setValue(xVal);
		this->ui.yVal->setValue(yVal);
		values[this->activePoint] = yVal;
		break;
	case 3:
		this->ui.yVal->setValue(yVal);
		values[this->activePoint] = yVal;
		break;
	}

	// enable signals again
	this->ui.xVal->blockSignals(false);
	this->ui.yVal->blockSignals(false);

	// set values
	this->curve.SetValues(values[0], values[1], values[2], values[3]);

	// emit signal
	emit this->ValueChanged(this->attr);
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleEnvelopeWidget::PointSelected( int point )
{
	// set selected point
	this->activePoint = point;

	// get values
	float values[4] = { this->curve.GetValues()[0], this->curve.GetValues()[1], this->curve.GetValues()[2], this->curve.GetValues()[3] };

	// disable signals again
	this->ui.xVal->blockSignals(true);
	this->ui.yVal->blockSignals(true);

	switch(point)
	{
	case 0:
		this->ui.xVal->setMinimum(0.0f);
		this->ui.xVal->setValue(0.0f);
		this->ui.xVal->setDisabled(true);
		this->ui.yVal->setValue(values[0]);
		break;
	case 1:
		this->ui.xVal->setMinimum(0.0f);
		this->ui.xVal->setMaximum(this->curve.GetKeyPos1());
		this->ui.xVal->setDisabled(false);
		this->ui.xVal->setValue(this->curve.GetKeyPos0());
		this->ui.yVal->setValue(values[1]);
		break;
	case 2:
		this->ui.xVal->setMinimum(this->curve.GetKeyPos0());
		this->ui.xVal->setMaximum(1.0f);
		this->ui.xVal->setDisabled(false);
		this->ui.xVal->setValue(this->curve.GetKeyPos1());
		this->ui.yVal->setValue(values[2]);
		break;
	case 3:
		this->ui.xVal->setMaximum(1.0f);
		this->ui.xVal->setValue(1.0f);
		this->ui.xVal->setDisabled(true);
		this->ui.yVal->setValue(values[3]);
		break;
	}

	// enable signals again
	this->ui.xVal->blockSignals(false);
	this->ui.yVal->blockSignals(false);

	// set values
	this->curve.SetValues(values[0], values[1], values[2], values[3]);
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleEnvelopeWidget::MaxValueChanged( double max )
{
	// get values
	float values[4] = { this->curve.GetValues()[0], this->curve.GetValues()[1], this->curve.GetValues()[2], this->curve.GetValues()[3] };

	// get limits
	float limits[2] = { this->curve.GetLimits()[0], this->curve.GetLimits()[1] };

	// calculate span
	float span = limits[1] - limits[0];

	int i;
	for(i = 0; i < 4; i++)
	{
		if (span > 0)
		{
			// remove span from values
			values[i] -= limits[0];
			values[i] /= span;
		}		
	}

	// update limits
	limits[1] = max;

	// calculate new span
	span = limits[1] - limits[0];

	// rescale points
	for(i = 0; i < 4; i++)
	{
		// get percentile values back
		values[i] *= span;
		values[i] += limits[0];
		//values[i] = n_clamp(values[i], limits[0], limits[1]);
	}

	// set limits
	this->curve.SetLimits(limits[0], limits[1]);

	// set values
	this->curve.SetValues(values[0], values[1], values[2], values[3]);

	// restrain min box
	this->ui.min->setMaximum(max);

	// block signals
	this->ui.yVal->blockSignals(true);

	// restrain yval
	this->ui.yVal->setMaximum(max);

	// set recalculated values
	this->ui.yVal->setValue(values[this->activePoint]);

	// enable signals again
	this->ui.yVal->blockSignals(false);

	// update points
	this->UpdatePoints();

	// emit signal
	emit this->ValueChanged(this->attr);
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleEnvelopeWidget::MinValueChanged( double min )
{
	// get values
	float values[4] = { this->curve.GetValues()[0], this->curve.GetValues()[1], this->curve.GetValues()[2], this->curve.GetValues()[3] };

	// get limits
	float limits[2] = { this->curve.GetLimits()[0], this->curve.GetLimits()[1] };

	// calculate span
	float span = limits[1] - limits[0];

	int i;
	for(i = 0; i < 4; i++)
	{
		if (span > 0)
		{
			// remove span from values
			values[i] -= limits[0];
			values[i] /= span;
		}		
	}

	// update limits
	limits[0] = min;

	// calculate new span
	span = limits[1] - limits[0];

	// rescale points
	for(i = 0; i < 4; i++)
	{
		// get percentile values back
		values[i] *= span;
		//values[i] = n_clamp(values[i], limits[0], limits[1]);
		values[i] += limits[0];
	}

	// set limits
	this->curve.SetLimits(limits[0], limits[1]);

	// set values
	this->curve.SetValues(values[0], values[1], values[2], values[3]);

	// restrain max box
	this->ui.max->setMinimum(min);

	// block signals
	this->ui.yVal->blockSignals(true);

	// restrain yval
	this->ui.yVal->setMinimum(min);

	// set recalculated values
	this->ui.yVal->setValue(values[this->activePoint]);

	// enable signals again
	this->ui.yVal->blockSignals(false);

	// update points
	this->UpdatePoints();

	// emit signal
	emit this->ValueChanged(this->attr);
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleEnvelopeWidget::FrequencyChanged( double freq )
{
	// change value
	this->curve.SetFrequency(freq);

	// emit signal
	emit this->ValueChanged(this->attr);
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleEnvelopeWidget::AmplitudeChanged( double amp )
{
	// change value
	this->curve.SetAmplitude(amp);

	// emit signal
	emit this->ValueChanged(this->attr);
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleEnvelopeWidget::FunctionChanged( int func )
{
	// change value
	this->curve.SetModFunc(func);

	// emit signal
	emit this->ValueChanged(this->attr);
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleEnvelopeWidget::UpdatePoints()
{
	// get values
	const float* values = curve.GetValues();
	const float* limits = curve.GetLimits();

	// calculate span
	float span = limits[1] - limits[0];

	// calculate points
	QPointF p0(0.0f, (values[0] - limits[0]) / span);
	QPointF p1(curve.GetKeyPos0(), (values[1] - limits[0]) / span);
	QPointF p2(curve.GetKeyPos1(), (values[2] - limits[0]) / span);
	QPointF p3(1.0f, (values[3] - limits[0]) / span);

	// setup from points
	this->ui.renderWidget->Setup(p0, p1, p2, p3);
}

} // namespace Particles
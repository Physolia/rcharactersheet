/***************************************************************************
* Copyright (C) 2014 by Renaud Guezennec                                   *
* http://www.rolisteam.org/                                                *
*                                                                          *
*  This file is part of rcse                                               *
*                                                                          *
* rcse is free software; you can redistribute it and/or modify             *
* it under the terms of the GNU General Public License as published by     *
* the Free Software Foundation; either version 2 of the License, or        *
* (at your option) any later version.                                      *
*                                                                          *
* rcse is distributed in the hope that it will be useful,                  *
* but WITHOUT ANY WARRANTY; without even the implied warranty of           *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the             *
* GNU General Public License for more details.                             *
*                                                                          *
* You should have received a copy of the GNU General Public License        *
* along with this program; if not, write to the                            *
* Free Software Foundation, Inc.,                                          *
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.                 *
***************************************************************************/
#include "tablefield.h"
#include <QPainter>
#include <QMouseEvent>
#include <QJsonArray>
#include <QUuid>
#include <QDebug>
#include "field.h"

#ifndef RCSE
TableCanvasField::TableCanvasField()
{

}
#endif
//////////////////////////////////////////
/// @brief LineFieldItem::createLineItem
/// @return
//////////////////////////////////////////



LineFieldItem::LineFieldItem(QObject *parent)
    : QObject(parent)
{

}

LineFieldItem::~LineFieldItem()
{

}

void LineFieldItem::insertField(Field *field)
{
    m_fields.append(field);
}

Field* LineFieldItem::getField(int k) const
{
    if(m_fields.size()>k)
        return m_fields.at(k);
    else
        return nullptr;
}

QList<Field *> LineFieldItem::getFields() const
{
    return m_fields;
}

void LineFieldItem::setFields(const QList<Field *> &fields)
{
    m_fields = fields;
}
int LineFieldItem::getFieldCount() const
{
    return m_fields.size();
}
Field* LineFieldItem::getFieldById(const QString& id)
{
   for(const auto field : m_fields)
   {
       if(field->getId() == id)
       {
           return field;
       }
   }
   return nullptr;
}
void LineFieldItem::save(QJsonArray &json)
{
    for(const auto field : m_fields)
    {
        QJsonObject obj;
        field->save(obj);
        json.append(obj);
    }
}
void LineFieldItem::load(QJsonArray &json, QList<QGraphicsScene *> scene,  CharacterSheetItem* parent)
{
    for( auto value : json)
    {
        Field* field = new Field();
        field->setParent(parent);
        QJsonObject obj = value.toObject();
        field->load(obj,scene);
        m_fields.append(field);
    }
}
////////////////////////////////////////
//
////////////////////////////////////////
LineModel::LineModel()
{

}
int LineModel::rowCount(const QModelIndex& parent) const
{
    return m_lines.size();
}

QVariant LineModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    auto item = m_lines.at(index.row());

    if(role == LineRole)
    {
        return QVariant::fromValue<LineFieldItem*>(item);
    }
    else
    {
        int key = role - (LineRole+1);
        return QVariant::fromValue<Field*>(item->getField(key/2));
    }
    return QVariant();
}

QHash<int, QByteArray>  LineModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[LineRole] = "line";
    int i = 1;
    auto first = m_lines.first();
    for(auto fieldLine : first->getFields() )
    {
        roles[LineRole+i]=fieldLine->getId().toUtf8();
        i++;
        roles[LineRole+i]=fieldLine->getLabel().toUtf8();
        i++;
    }
    return roles;
}
void LineModel::insertLine(LineFieldItem* line)
{
    beginInsertRows(QModelIndex(),m_lines.size(),m_lines.size());
    m_lines.append(line);
    endInsertRows();
}
void LineModel::clear()
{
    beginResetModel();
    qDeleteAll(m_lines);
    m_lines.clear();
    endResetModel();
}
int LineModel::getChildrenCount() const
{
    if(!m_lines.isEmpty())
    {
        return m_lines.size()*getColumnCount();
    }
    return 0;
}
Field*  LineModel::getFieldById(const QString& id)
{
    Field* item = nullptr;
    for(const auto& line : m_lines)
    {
        item = line->getFieldById(id);
    }
    return item;

}
int LineModel::getColumnCount() const
{
    if(!m_lines.isEmpty())
    {
        auto line = m_lines.first();
        return line->getFieldCount();
    }
    return 0;
}
Field* LineModel::getField(int line, int col)
{
    if(m_lines.size()>line)
    {
        return m_lines.at(line)->getField(col);
    }
    return nullptr;
}
void LineModel::save(QJsonArray &json)
{
    for(const auto& line : m_lines)
    {
        QJsonArray lineJson;
        line->save(lineJson);
        json.append(lineJson);
    }
}
void LineModel::load(QJsonArray &json, QList<QGraphicsScene *> scene, CharacterSheetItem* parent)
{
    QJsonArray::Iterator it;
    for(it = json.begin(); it != json.end(); ++it)
    {
        QJsonArray obj = (*it).toArray();
        LineFieldItem* line = new LineFieldItem();
        line->load(obj,scene,parent);
        m_lines.append(line);
    }
}
///////////////////////////////////
/// \brief TableField::TableField
/// \param addCount
/// \param parent
///////////////////////////////////
TableField::TableField(bool addCount,QGraphicsItem* parent)
    : Field(addCount,parent)
{
    init();
}

TableField::TableField(QPointF topleft,bool addCount,QGraphicsItem* parent)
    : Field(topleft,addCount,parent)
{
    Q_UNUSED(topleft);
    m_value = QStringLiteral("value");
    init();
}
TableField::~TableField()
{
#ifdef RCSE
    if(nullptr!=m_tableCanvasField)
    {
        delete m_tableCanvasField;
    }
    m_canvasField = nullptr;
    m_tableCanvasField = nullptr;
#endif
}

LineModel *TableField::getModel() const
{
    return m_model;
}

void TableField::addLine()
{
    auto lineItem = new LineFieldItem(this);
    auto index = m_model->index(0,0);
    auto first = m_model->data(index,LineModel::LineRole).value<LineFieldItem*>();
    for(Field* field : first->getFields())
    {
        if(nullptr!=field)
        {
            Field* newField = new Field();
            newField->copyField(field,true);
            newField->setParent(field->getParent());
            lineItem->insertField(newField);
        }
    }
    m_model->insertLine(lineItem);
}

void TableField::removeLine(int)
{

}
void TableField::init()
{
    m_canvasField = nullptr;
    m_tableCanvasField = nullptr;
    m_id = QStringLiteral("id_%1").arg(m_count);
    m_currentType=Field::TABLE;
    m_clippedText = false;
    m_model = new LineModel();


    m_border=NONE;
    m_textAlign = Field::TopLeft;
    m_bgColor = Qt::transparent;
    m_textColor = Qt::black;
    m_font = font();
}

TableField::ControlPosition TableField::getPosition() const
{
    return m_position;
}

void TableField::setPosition(const ControlPosition &position)
{
    m_position = position;
}

void TableField::setCanvasField(CanvasField *canvasField)
{
    m_tableCanvasField = dynamic_cast<TableCanvasField*>(canvasField);
    Field::setCanvasField(canvasField);
}

bool TableField::hasChildren()
{
    if(m_model->rowCount(QModelIndex())>0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int TableField::getChildrenCount() const
{
   return m_model->getChildrenCount();
}

CharacterSheetItem* TableField::getChildAt(QString id)
{
    return m_model->getFieldById(id);
}

CharacterSheetItem* TableField::getChildAt(int index) const
{
    int itemPerLine = m_model->getColumnCount();
    int line = index/itemPerLine;
    int col = index - (line*itemPerLine);
    return m_model->getField(line,col);
}

void TableField::save(QJsonObject &json, bool exp)
{
    if(exp)
    {
        json["type"]="TableField";
        json["id"]=m_id;
        json["label"]=m_label;
        json["value"]=m_value;
        return;
    }
    json["type"]="TableField";
    json["id"]=m_id;
    json["typefield"]=m_currentType;
    json["label"]=m_label;
    json["value"]=m_value;
    json["border"]=m_border;
    json["page"]=m_page;
    json["formula"]=m_formula;

    json["clippedText"]=m_clippedText;

    QJsonObject bgcolor;
    bgcolor["r"]=QJsonValue(m_bgColor.red());
    bgcolor["g"]=m_bgColor.green();
    bgcolor["b"]=m_bgColor.blue();
    bgcolor["a"]=m_bgColor.alpha();
    json["bgcolor"]=bgcolor;

    json["positionControl"] = m_position;

    QJsonObject textcolor;
    textcolor["r"]=m_textColor.red();
    textcolor["g"]=m_textColor.green();
    textcolor["b"]=m_textColor.blue();
    textcolor["a"]=m_textColor.alpha();
    json["textcolor"]=textcolor;

    json["font"]=m_font.toString();
    json["textalign"]=m_textAlign;
    json["x"]=getValueFrom(CharacterSheetItem::X,Qt::DisplayRole).toDouble();
    json["y"]=getValueFrom(CharacterSheetItem::Y,Qt::DisplayRole).toDouble();
    json["width"]=getValueFrom(CharacterSheetItem::WIDTH,Qt::DisplayRole).toDouble();
    json["height"]=getValueFrom(CharacterSheetItem::HEIGHT,Qt::DisplayRole).toDouble();
    QJsonArray valuesArray;
    valuesArray=QJsonArray::fromStringList(m_availableValue);
    json["values"]=valuesArray;

    QJsonArray childArray;
    m_model->save(childArray);
    json["children"]=childArray;

    #ifdef RCSE
    if(nullptr != m_tableCanvasField)
    {
        QJsonObject obj;
        m_tableCanvasField->save(obj);
        json["canvas"]=obj;
    }
    #endif

}

void TableField::load(QJsonObject &json, QList<QGraphicsScene *> scene)
{
    m_id = json["id"].toString();
    m_border = static_cast<BorderLine>(json["border"].toInt());
    m_value= json["value"].toString();
    m_label = json["label"].toString();

    m_currentType=static_cast<Field::TypeField>(json["type"].toInt());
    m_clippedText=json["clippedText"].toBool();

    m_formula = json["formula"].toString();

    QJsonObject bgcolor = json["bgcolor"].toObject();
    int r,g,b,a;
    r = bgcolor["r"].toInt();
    g = bgcolor["g"].toInt();
    b = bgcolor["b"].toInt();
    a = bgcolor["a"].toInt();

    m_bgColor=QColor(r,g,b,a);

    QJsonObject textcolor = json["textcolor"].toObject();

    r = textcolor["r"].toInt();
    g = textcolor["g"].toInt();
    b = textcolor["b"].toInt();
    a = textcolor["a"].toInt();



    m_textColor=QColor(r,g,b,a);

    m_position = static_cast<ControlPosition>(json["positionControl"].toInt());

    m_font.fromString(json["font"].toString());

    m_textAlign = static_cast<Field::TextAlign>(json["textalign"].toInt());
    qreal x,y,w,h;
    x=json["x"].toDouble();
    y=json["y"].toDouble();
    w=json["width"].toDouble();
    h=json["height"].toDouble();
    m_page=json["page"].toInt();

    QJsonArray valuesArray=json["values"].toArray();
    for(auto value : valuesArray.toVariantList())
    {
        m_availableValue << value.toString();
    }
    m_rect.setRect(x,y,w,h);
    #ifdef RCSE
    QJsonArray childArray=json["children"].toArray();
    m_model->load(childArray,scene,this);

    if(json.contains("canvas"))
    {
        m_tableCanvasField = new TableCanvasField(this);
        auto obj = json["canvas"].toObject();
        m_tableCanvasField->load(obj,scene);
        m_canvasField = m_tableCanvasField;
    }
    m_canvasField->setPos(x,y);
    m_canvasField->setWidth(w);
    m_canvasField->setHeight(h);
    #endif

}


bool TableField::mayHaveChildren() const
{
    return true;
}
void TableField::generateQML(QTextStream &out,CharacterSheetItem::QMLSection sec,int i, bool isTable)
{
    #ifdef RCSE
    Q_UNUSED(i)
    Q_UNUSED(isTable)

    if(nullptr==m_tableCanvasField)
    {
        return;
    }

    m_model->clear();

    m_tableCanvasField->fillLineModel(m_model,this);
    emit updateNeeded(this);
    if(sec==CharacterSheetItem::FieldSec)
    {
        out << "    ListView{//"<< m_label <<"\n";
        out << "        id: _" << m_id<<"list\n";
        out << "        x:" << m_tableCanvasField->pos().x() << "*root.realscale"<<"\n";
        out << "        y:" <<  m_tableCanvasField->pos().y()<< "*root.realscale"<<"\n";
        out << "        width:" << m_tableCanvasField->boundingRect().width() <<"*root.realscale"<<"\n";
        out << "        height:"<< m_tableCanvasField->boundingRect().height()<<"*root.realscale"<<"\n";
        if(m_page>=0)
        {
            out << "    visible: root.page == "<< m_page << "? true : false\n";
        }
        out << "        readonly property int maxRow:"<<m_tableCanvasField->lineCount() << "\n";
        out << "        interactive: count>maxRow?true:false;\n";
        out << "        clip: true;\n";
        out << "        model:"<< m_id << ".model\n";
        out << "        delegate: RowLayout {\n";
        out << "            height: _"<< m_id<<"list.height/_"<< m_id<<"list.maxRow\n";
        out << "            width:  _"<< m_id<<"list.width\n";
        out << "            spacing:0\n";
        m_tableCanvasField->generateSubFields(out);
        out << "        }\n";
        out << "     }\n";
        out << "     Button {\n";
        out << "        anchors.top: _" << m_id<<"list.bottom\n";
        out << "        text: \""<< tr("Add line") << "\"\n";
        out << "        onClicked: "<< m_id <<".addLine()\n";
        out << "     }\n";
    }
     #endif
}


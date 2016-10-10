/*
Copyright 2016 Tyler Gilbert

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#include <QFileInfo>
#include <QDebug>

#include "Helper.h"

using namespace StratifyIO;

QString Helper::convertStringListToJson(const QStringList & list, const QString & key){
    QJsonObject jsonObject;
    QJsonArray jsonArray;


    foreach(QString entry, list){
        //look for the settings file inside the directory -- if not found -- don't open
        QJsonObject object;
        object.insert(key, QJsonValue(entry));
        jsonArray.push_back(object);
    }

    jsonObject.insert("data", jsonArray);


    return QJsonDocument(jsonObject).toJson();
}

bool Helper::doesFileExist(const QString & dir, const QString & name){
    QFileInfo info;
    info.setFile(dir + "/" + name);
    return info.exists();
}


int Helper::son_add_json_value(son_t * son, const QJsonValue & value, const QString & valueKey){
    int ret = 0;
    const char * k = valueKey.toStdString().c_str();

    switch(value.type()){
    case QJsonValue::Null: ret = son_write_null(son, k); break;
    case QJsonValue::Bool:
        if( value.toBool() ){
            ret = son_write_true(son, k);
        } else {
            ret = son_write_false(son, k);
        }
        break;
    case QJsonValue::Double:
        ret = son_write_float(son, k, (float)value.toDouble());
        break;
    case QJsonValue::String:
        ret = son_write_str(son, k, value.toString().toStdString().c_str());
        break;
    case QJsonValue::Array:
        ret = son_add_json_array(son, value.toArray(), valueKey);
        break;
    case QJsonValue::Object:
        ret = son_add_json_object(son, value.toObject(), valueKey);
        break;
    case QJsonValue::Undefined:

        break;
    }
    return ret;
}

int Helper::son_add_json_array(son_t * son, const QJsonArray & array, const QString & arrayKey){
    int ret = 0;
    if( son_open_array(son, arrayKey.toStdString().c_str(), 1) < 0 ){
        return -1;
    }


    for(int i = 0; i < array.size(); i++){
        if( son_add_json_value(son, array.at(i), QString::number(i)) < 0 ){
            ret = -1;
            break;
        }
    }

    son_close_array(son);
    return ret;
}

int Helper::son_add_json_object(son_t * son, const QJsonObject & object, const QString & objectKey){
    int ret = 0;
    if( son_open_obj(son, objectKey.toStdString().c_str()) < 0 ){
        return -1;
    }

    foreach(QString key, object.keys()){

        //QJsonValue value = object.value(key);
        if( son_add_json_value(son, object.value(key), key) < 0 ){
            ret = -1;
            break;
        }

    }

    son_close_obj(son);
    return ret;
}

int Helper::son_create_from_json(const QString & dest, const QString & source, int son_stack_size){
    QFile inputFile;
    int ret = 0;

    son_stack_t son_stack[son_stack_size];
    son_t son;



    inputFile.setFileName(source);

    if( inputFile.open(QFile::ReadOnly) == false ){
        qDebug() << "Failed to open" << source;
        return -1;
    }

    QJsonDocument doc = QJsonDocument::fromJson(inputFile.readAll());
    inputFile.close();

    QJsonObject object = doc.object();

    son_set_driver(&son, 0);

    if( son_create(&son, dest.toStdString().c_str(), son_stack, son_stack_size) < 0 ){
        qDebug() << "Failed to create" << dest;
        return -1;
    }

    if( son_add_json_object(&son, object, "root") < 0 ){
        qDebug() << "Failed to add root object";
        ret = -1;
    }

    son_close(&son, 1);

    return ret;

}
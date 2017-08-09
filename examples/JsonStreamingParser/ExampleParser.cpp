#include "ExampleParser.h"
#include "JsonListener.h"


void ExampleListener::whitespace(char c) {
  Serial.println("whitespace");
}

void ExampleListener::startDocument() {
  Serial.println("start document");
}

void ExampleListener::key(const char *key) {
  Serial.print("key: ");
  Serial.println(key);
}

void ExampleListener::value(const char *value) {
  Serial.print("value: ");
  Serial.print(value);
}

void ExampleListener::endArray() {
  Serial.println("end array. ");
}

void ExampleListener::endObject() {
  Serial.println("end object. ");
}

void ExampleListener::endDocument() {
  Serial.println("end document. ");
}

void ExampleListener::startArray() {
   Serial.println("start array. ");
}

void ExampleListener::startObject() {
   Serial.println("start object. ");
}


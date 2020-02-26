# Loadable Modules - Add your own custom features to Mesibo

This repository contains the Sample Loadable Modules that you can use as a reference, to quickly see Mesibo Modules in action! 

The complete documentation for Mesibo Modules is available [here](https://mesibo.com/documentation/on-premise/loadable-modules/)

## List of Sample Modules 

- [Skeleton](https://github.com/mesibo/onpremise-loadable-modules/tree/master/skeleton) Barebones version of a Mesibo Module that explains the usage of different aspects of the module, various callback functions, callable functions, and utilities.

- [Filter](https://github.com/mesibo/onpremise-loadable-modules/tree/master/filter) Filter module to monitor and moderate all the messages between your users (for example, to drop messages containing profanity)

- [Translate](https://github.com/mesibo/onpremise-loadable-modules/tree/master/translate) Translator module to translate each message before sending it to destination. Sample translate Module uses [Google Translate](https://cloud.google.com/translate) for translation.

- [Chatbot](https://github.com/mesibo/onpremise-loadable-modules/tree/master/chatbot) Chatbot module to analyze messages using various AI and machine learning tools like Tensorflow, Dialogflow, etc. and send an automatic reply. Sample Chatbot Module uses [Dialogflow](https://dialogflow.com)

- [FCGI](https://github.com/mesibo/onpremise-loadable-modules/tree/master/fcgi) FCGI module to interface with a messaging backend and run scripts on your web server. 

- [V8](https://github.com/mesibo/onpremise-loadable-modules/tree/master/js) V8 Module to load and call functions in [ECMAScript](http://www.ecma-international.org/ecma-262/5.1/). Sample V8 Module uses the embeddable JS Engine [V8](https://v8.dev)

## Compiling Modules
To compile a Mesibo module, go to module directory and run make from the module directory.

```
sudo make
```

On the successful build of your module, verify that the target path should contain your shared library. Example, `/usr/lib64/mesibo/mesibo_mod_<module name>.so`

## Loading Modules
To load a Mesibo module provide the configuration in `/etc/mesibo/mesibo.conf`. You can copy the configuration from `sample.conf` provided in each repo, into `/etc/mesibo/mesibo.conf` and modify values accordingly. 

Mount the directory `/path/to/mesibo_mod_<module name>.so` containing your module(.so file) while running the mesibo container. For example, if the module is located at `/usr/lib64/mesibo/`


```
sudo docker run  -v /certs:/certs -v  /usr/lib64/mesibo/:/usr/lib64/mesibo/ \
         -v /etc/mesibo:/etc/mesibo -net host -d mesibo/mesibo <app token> 
```

Refer [Mesibo Modules Documentation](https://mesibo.com/documentation/on-premise/loadable-modules/) for more details.

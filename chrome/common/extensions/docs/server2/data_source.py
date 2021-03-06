# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class DataSource(object):
  '''
  Defines an abstraction for all DataSources.

  DataSources must have two public methods, get and Refresh. A DataSource is
  initialized with a ServerInstance and a Request (defined in servlet.py).
  Anything in the ServerInstance can be used by the DataSource. Request is None
  when DataSources are created for Refresh.

  DataSources are used to provide templates with access to data. DataSources may
  not access other DataSources and any logic or data that is useful to other
  DataSources must be moved to a different class.
  '''
  def __init__(self, server_instance, request):
    pass

  def GetRefreshPaths(self):
    '''Returns a list of paths to query
    (relative to _refresh/<data_source_name>/) with the task queue in order
    to refresh this DataSource's data set. Any paths listed here will be
    routed to the DataSource Refresh method in a taskqueue task request.
    '''
    return ['']

  def Refresh(self, path=None):
    '''Handles _refresh requests to this DataSource. Should return a Future
    indicating the success or failure of the refresh.'''
    raise NotImplementedError(self.__class__)

  def get(self, key):
    '''Returns a dictionary or list that can be consumed by a template. Called
    on an offline file system and can only access files in the cache.
    '''
    raise NotImplementedError(self.__class__)

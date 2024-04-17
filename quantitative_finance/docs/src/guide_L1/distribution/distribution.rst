
.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

************************
Probability Distribution
************************

Overview
========

In probability theory and statistics, a probability distribution is a mathematical function that provides the probabilities of occurrence of different possible outcomes in an experiment. In more technical terms, the probability distribution is a description of a random phenomenon in terms of the probabilities of events (from wiki). Here, probability distributions implement probability density function (PDF), probability mass funciton (PMF), cumulative distribution function (CDF), and inverse cumulative distribution function (ICDF). The details are listed below.


.. _tabDist:

.. table:: the detail about the distribution
    :align: center

    +-------------------+-------------+----------+----------+-----------------------+
    |   Distribution    |   PDF/PMF   |    CDF   |   ICDF   |  Reference Algorithm  |
    +-------------------+-------------+----------+----------+-----------------------+
    |     Bernoulli     |      Y      |     Y    |          |    `Bernoulli ref`_   |
    +-------------------+-------------+----------+----------+-----------------------+
    |     Binomial      |      Y      |     Y    |          |    `Binomial ref`_    | 
    +-------------------+-------------+----------+----------+-----------------------+
    |     Normal        |      Y      |     Y    |     Y    |    `Normal ref`_      |
    +-------------------+-------------+----------+----------+-----------------------+
    |     Lognormal     |      Y      |     Y    |     Y    |    `Lognormal ref`_   |
    +-------------------+-------------+----------+----------+-----------------------+
    |     Poisson       |      Y      |     Y    |     Y    |    `Poisson ref`_     |
    +-------------------+-------------+----------+----------+-----------------------+
    |     Gamma         |             |     Y    |          |    `Gamma ref`_       |
    +-------------------+-------------+----------+----------+-----------------------+
    note: "Y" indicates that the function is implemented.

.._`Bernoulli ref`: https://en.wikipedia.org/wiki/Bernoulli_distribution

.._`Binomial ref`: https://en.wikipedia.org/wiki/Binomial_distribution

.._`Normal ref`: https://en.wikipedia.org/wiki/Log-normal_distribution

.._`Lognormal ref`: https://en.wikipedia.org/wiki/Log-normal_distribution

.._`Poisson ref` https://en.wikipedia.org/wiki/Poisson_distribution

.._`Gamma ref`: https://en.wikipedia.org/wiki/Gamma_distribution



.. toctree::
   :maxdepth: 1

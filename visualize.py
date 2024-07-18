import matplotlib.pyplot as plt
import numpy as np
import scipy.stats as stats
import pandas as pd

# Read the CSV output from the C program
df = pd.read_csv('battleship_results.csv')

# Separate results for each strategy
random_results = df[df['strategy'] == 'Random']['average_shots'].values
pdf_results = df[df['strategy'] == 'PDF']['average_shots'].values
hunt_target_results = df[df['strategy'] == 'Hunt and Target']['average_shots'].values

strategies = ['Random', 'PDF', 'Hunt and Target']
means = [np.mean(random_results), np.mean(pdf_results), np.mean(hunt_target_results)]
stds = [np.std(random_results), np.std(pdf_results), np.std(hunt_target_results)]

# Bar chart
plt.figure(figsize=(10, 6))
plt.bar(strategies, means, yerr=stds, capsize=5)
plt.title('Average Shots to Win by Strategy')
plt.ylabel('Number of Shots')
plt.savefig('strategy_comparison.png')
plt.close()

# Box plot
plt.figure(figsize=(10, 6))
plt.boxplot([random_results, pdf_results, hunt_target_results], labels=strategies)
plt.title('Distribution of Shots to Win by Strategy')
plt.ylabel('Number of Shots')
plt.savefig('strategy_distribution.png')
plt.close()

# T-tests
t_random_pdf, p_random_pdf = stats.ttest_ind(random_results, pdf_results)
t_random_hunt, p_random_hunt = stats.ttest_ind(random_results, hunt_target_results)
t_pdf_hunt, p_pdf_hunt = stats.ttest_ind(pdf_results, hunt_target_results)

print(f"T-test results:")
print(f"Random vs PDF: t={t_random_pdf:.4f}, p={p_random_pdf:.4e}")
print(f"Random vs Hunt and Target: t={t_random_hunt:.4f}, p={p_random_hunt:.4e}")
print(f"PDF vs Hunt and Target: t={t_pdf_hunt:.4f}, p={p_pdf_hunt:.4e}")